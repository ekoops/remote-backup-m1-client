#include <boost/archive/binary_oarchive.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/thread.hpp>
#include <algorithm>
#include "connection.h"
#include "f_message.h"
#include "tlv_view.h"

namespace ssl = boost::asio::ssl;

/**
 * Construct a connection instance with an associated
 * SSL socket and a thread pool to execute related completion handlers.
 *
 * @param io io_context object associated with socket operation
 * @param ctx ssl context object, used to specify SSL connection parameters
 * @param thread_pool_size the number of threads inside the thread pool
 * @return a new constructed connection instance
 */
connection::connection(boost::asio::io_context &io,
                       ssl::context &ctx,
                       size_t thread_pool_size
) : ex_work_guard_{boost::asio::make_work_guard(io)},
    strand_{boost::asio::make_strand(io)},
    socket_{strand_, ctx},
    resolver_{strand_},
    keepalive_timer_{io, boost::asio::chrono::seconds{30}} {
    this->socket_.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);

    this->thread_pool_.reserve(thread_pool_size);
    for (int i = 0; i < thread_pool_size; i++) {
        this->thread_pool_.emplace_back(boost::bind(&boost::asio::io_context::run, &io), std::ref(io));
    }
}


void connection::resolve(std::string const &hostname, std::string const &service) {
    boost::system::error_code ec;
    this->endpoints_ = this->resolver_.resolve(hostname, service, ec);
    if (ec) {
        std::cerr << "Error in resolve():\n\t" << ec.message() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

/**
 * Construct a connection instance std::shared_ptr with an associated
 * SSL socket and a thread pool to execute related completion handlers.
 *
 * @param io io_context object associated with socket operation
 * @param ctx ssl context object, used to specify SSL connection parameters
 * @param thread_pool_size the number of threads inside the thread pool
 * @return a new constructed connection instance std::shared_ptr
 */
std::shared_ptr<connection> connection::get_instance(boost::asio::io_context &io,
                                                     ssl::context &ctx,
                                                     size_t thread_pool_size) {
    return std::shared_ptr<connection>(new connection{io, ctx, thread_pool_size});
}

/**
 * Allow to establish an SSL socket connection with the specified endpoint
 *
 * @param hostname the desired endpoint hostname
 * @param service the desired endpoint service/port number
 * @return void
 */
void connection::connect() {
    SSL_clear(this->socket_.native_handle());
    boost::system::error_code ec;
    do {
        boost::asio::connect(this->socket_.lowest_layer(), this->endpoints_, ec);
        if (ec) {
            std::cerr << "Failed to connect. Retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        }
    } while (ec);
    this->socket_.lowest_layer().set_option(boost::asio::socket_base::keep_alive(true));
    boost::system::error_code hec;
    this->socket_.handshake(ssl::stream<boost::asio::ip::tcp::socket>::client, hec);
    if (hec) std::exit(EXIT_FAILURE);
    if (this->user_.authenticated() &&      // if user is already authenticated
        !this->auth(this->user_) &&      // and the auth message server response is not good
        !this->login()) {                   // and the user fails the login procedure
        std::exit(EXIT_FAILURE);
    }
}

bool connection::login() {
    std::string username, password;
//    boost::regex username_regex{"[a-z][a-z\\d_\\.]{7, 15}"};
//    boost::regex password_regex{"[a-zA-Z0-9\\d\\-\\.@$!%*?&]{8, 16}"};
    boost::regex username_regex{".*"};
    boost::regex password_regex{".*"};
    int field_attempts = 3;
    int general_attempts = 3;

    while (general_attempts) {
        std::cout << "Insert your username:" << std::endl;
        while (!(std::cin >> username) || !boost::regex_match(username, username_regex)) {
            std::cin.clear();
            std::cout << "Failed to get username. Try again (attempts left " << --field_attempts << ")."
                      << std::endl;
            if (!field_attempts) return false;
        }
        field_attempts = 0;
        std::cout << "Insert your password:" << std::endl;
        while (!(std::cin >> password) || !boost::regex_match(password, password_regex)) {
            std::cin.clear();
            std::cout << "Failed to get password. Try again (attempts left " << --field_attempts << ")."
                      << std::endl;
            if (!field_attempts) return false;
        }
        user usr{username, password};
        if (this->auth(usr)) {
            this->user_ = usr;
            return true;
        } else std::cout << "Authentication failed (attempts left " << --general_attempts << ")." << std::endl;
    }
    return false;
}

/**
 * Allow to try to authenticate client user
 *
 * @param username the client user username
 * @param password the client user password
 * @return true if the user has been successfully authenticated, false otherwise
 */
bool connection::auth(user &usr) {
    std::string const &username = usr.username();
    std::string const &password = usr.password();
    communication::message auth_msg{communication::MESSAGE_TYPE::AUTH};
    auth_msg.add_TLV(communication::TLV_TYPE::USRN, username.size(), username.c_str());
    auth_msg.add_TLV(communication::TLV_TYPE::PSWD, password.size(), password.c_str());
    auth_msg.add_TLV(communication::TLV_TYPE::END);

    auto response = this->sync_post(auth_msg);
    if (!response) return false;
    communication::tlv_view view{response.value()};
    if (view.next_tlv() && view.tlv_type() == communication::TLV_TYPE::OK) {
        usr.authenticated(true);
        return true;
    } else return false;
}

void connection::schedule_keepalive() {
    try {
        this->keepalive_timer_.expires_after(boost::asio::chrono::seconds{30});
        this->keepalive_timer_.async_wait(
                boost::asio::bind_executor(
                this->strand_,
                [this](boost::system::error_code const& e) {
                    if (!e) {
                        std::cout << "Sending keep alive message..." << std::endl;
                        communication::message msg{communication::MESSAGE_TYPE::KEEP_ALIVE};
                        msg.add_TLV(communication::TLV_TYPE::END);
                        this->write(msg);
                    }
                }
        ));
    }
    catch (boost::system::system_error &e) {
        std::cerr << "Failed to set keepalive timer" << std::endl;
    }
}

std::optional<communication::message> connection::sync_post(
        communication::message const &request_msg
) {
    return this->write(request_msg) ? this->read() : std::nullopt;
}


/**
 * Allow to send a message and handle server response using
 * a specified callback.
 *
 * @param request_msg message that has to be sent
 * @param fn callback for handling server response
 * @return void
 */
void connection::post(
        communication::message const &request_msg,
        std::function<void(std::optional<communication::message> const &)> const &fn
) {
    boost::asio::post(this->strand_, [this, request_msg, fn]() {
        fn(sync_post(request_msg));
    });
}

/**
 * Allow to send an f_message through different sequential
 * message to server and handle server response using
 * a specified callback.
 *
 * @param request_msg f_message that has to be sent
 * @param fn callback for handling server response
 * @return void
 */
void connection::post(const std::shared_ptr<communication::f_message> &request_msg,
                      std::function<void(std::optional<communication::message> const &)> const &fn) {
    boost::asio::post(this->strand_, [this, request_msg, fn]() {
        std::optional<communication::message> msg;
        try {
            while (request_msg->next_chunk()) {
                if (!this->write(*request_msg) || !(msg = this->read())) {
                    fn(std::nullopt);
                }
            }
            fn(msg);
        }
        catch (std::exception &e) {
            fn(std::nullopt);
        }
    });
}

/**
 * Allow to send a message to server.
 *
 * @param request_msg message that has to be sent
 * @return true if the message has been written successfully, false otherwise
 */
bool connection::write(communication::message const &request_msg) {
    this->keepalive_timer_.cancel();
    std::cout << "START WRITE" << std::endl;
    size_t header = request_msg.raw_msg_ptr()->size();
    std::vector<boost::asio::mutable_buffer> buffers;
    buffers.emplace_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.emplace_back(request_msg.buffer());
    try {
        std::cout << "<<<<<<<<<<REQUEST>>>>>>>>>" << std::endl;
        std::cout << "HEADER: " << header << std::endl;
        std::cout << request_msg;
        boost::asio::write(this->socket_, buffers);
        this->schedule_keepalive();
        return true;
    }
    catch (boost::system::system_error &ex) {
        auto error_code = ex.code();
        if (error_code == boost::asio::error::eof ||
            error_code == boost::asio::error::connection_reset ||
            error_code == boost::asio::error::broken_pipe) {
            std::cerr << "WRITE: Connection to the server has been lost. Trying to reconnect..." << std::endl;
            this->connect();
        }
        return false;
    }
    catch (std::exception &ex) {
        std::cerr << "Error in write():\n\t" << ex.what() << std::endl;
        this->schedule_keepalive();
        return false;
    }
}

/**
 * Allow to read a message from server.
 *
 * @return an std::optional<communication::message> if a message has been read successfully,
 * true if the message has been written successfully, std::nullopt otherwise
 */
std::optional<communication::message> connection::read() {
    std::cout << "START READ" << std::endl;
    size_t header = 0;
    auto raw_msg_ptr = std::make_shared<std::vector<uint8_t>>();
    auto insert_pos = raw_msg_ptr->begin();
    bool ended = false;
    communication::MESSAGE_TYPE msg_type;
    bool first = true;
    try {
        this->keepalive_timer_.cancel();
        do {
            boost::asio::read(this->socket_, boost::asio::buffer(&header, sizeof(header)));
            if (!first) {
                header--;
                boost::asio::read(this->socket_, boost::asio::buffer(&msg_type, 1));
            }
            raw_msg_ptr->resize(raw_msg_ptr->size() + header);
            insert_pos = raw_msg_ptr->end();
            std::advance(insert_pos, -header);
            boost::asio::read(this->socket_, boost::asio::buffer(&*insert_pos, header));
            if (first) first = false;
            communication::message temp_msg{raw_msg_ptr};
            communication::tlv_view view{temp_msg};
            if (view.verify_end()) ended = true;
        } while (!ended);
        this->schedule_keepalive();
        communication::message response_msg{raw_msg_ptr};
        std::cout << "<<<<<<<<<<RESPONSE>>>>>>>>>" << std::endl;
        std::cout << "HEADER: " << response_msg.size() << std::endl;
        std::cout << response_msg;
        return std::make_optional<communication::message>(raw_msg_ptr);
    }
    catch (boost::system::system_error &ex) {
        auto error_code = ex.code();
        if (error_code == boost::asio::error::eof ||
            error_code == boost::asio::error::connection_reset ||
            error_code == boost::asio::error::broken_pipe) {
            std::cerr << "READ: Connection to the server has been lost. Trying to reconnect..." << std::endl;
            this->connect();
        }
        return std::nullopt;
    }
    catch (std::exception &ex) {
        std::cerr << "Error in read():\n\t" << ex.what() << std::endl;
        this->schedule_keepalive();
        return std::nullopt;
    }
}

/**
 * Allow to gracefully close server connection and
 * shutdown all connection's threads.
 *
 * @return void
 */
void connection::close() {
    try {
        // Waiting until threads ends the execution
        // of completion handlers
        for (auto &t : this->thread_pool_) if (t.joinable()) t.join();

        // Sending close_notify SSL message
        this->socket_.shutdown();
        // Sending FIN message
        this->socket_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        // Close the socket
        this->socket_.lowest_layer().close();
    }
    catch (boost::system::system_error &ex) {
        std::cerr << "Failed to gracefully close connection: " << ex.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    catch (std::exception &ex) {
        std::cerr << "Error in close():\n\t" << ex.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}