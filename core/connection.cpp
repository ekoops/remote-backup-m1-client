#include <boost/asio/bind_executor.hpp>
#include <boost/thread.hpp>
#include <algorithm>
#include <utility>
#include "connection.h"
#include "../communication/tlv_view.h"

namespace ssl = boost::asio::ssl;

/**
 * Construct a connection instance with an associated
 * SSL socket and a thread to execute related completion handlers.
 *
 * @param io io_context object associated with socket operation
 * @param ctx ssl context object, used to specify SSL connection parameters
 * @param thread_pool_size the number of threads inside the thread
 * @return a new constructed connection instance
 */
connection::connection(
        boost::asio::io_context &io,
        ssl::context &ctx
) : ex_work_guard_{boost::asio::make_work_guard(io)},
    strand_{boost::asio::make_strand(io)},
    socket_{strand_, ctx},
    resolver_{strand_},
    keepalive_timer_{strand_, boost::asio::chrono::seconds{30}},
    thread_{boost::bind(&boost::asio::io_context::run, &io), std::ref(io)} {

    this->socket_.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);

//    this->thread_pool_.reserve(1);
//    for (int i = 0; i < 1; i++) {
//        this->thread_pool_.emplace_back(boost::bind(&boost::asio::io_context::run, &io), std::ref(io));
//    }
}

/**
 * Construct a connection instance std::shared_ptr with an associated
 * SSL socket and a thread to execute related completion handlers.
 *
 * @param io io_context object associated with socket operation
 * @param ctx ssl context object, used to specify SSL connection parameters
 * @param thread_pool_size the number of threads inside the thread
 * @return a new constructed connection instance std::shared_ptr
 */
std::shared_ptr<connection> connection::get_instance(
        boost::asio::io_context &io,
        ssl::context &ctx
) {
    return std::shared_ptr<connection>(new connection{io, ctx});
}


/**
 * Allow to cancel the already scheduled keepalive timer
 *
 * @return void
 */
void connection::cancel_keepalive() {
    boost::system::error_code ec;
    this->keepalive_timer_.cancel(ec);
}

/**
 * Allow to schedule a 30 seconds keepalive timer
 *
 * @return void
 */
void connection::schedule_keepalive() {
    try {
        this->keepalive_timer_.expires_after(boost::asio::chrono::seconds{30});
        this->keepalive_timer_.async_wait(
                [this](boost::system::error_code const &e) {
                    if (!e) {
                        communication::message msg{communication::MSG_TYPE::KEEP_ALIVE};
                        msg.add_TLV(communication::TLV_TYPE::END);
                        auto result = this->sync_post(msg);
                    } else if (e != boost::asio::error::operation_aborted) {
                        return schedule_keepalive();
                    }
                }
        );
    }
    catch (boost::system::system_error &e) {
        std::cerr << "Failed to set keepalive timer" << std::endl;
    }
}

/**
 * Allow to obtain the available endpoints for a given hostname and service
 *
 * @param hostname the hostname that has to be resolved
 * @param service the service that has to be resolved
 *
 * @return void
 */
void connection::resolve(std::string const &hostname, std::string const &service) {
    boost::system::error_code ec;
    this->endpoints_ = this->resolver_.resolve(hostname, service, ec);
    if (ec) {
        std::cerr << "Error in resolve():\n\t" << ec.message() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

/**
 * Allow to establish an SSL socket connection for the first available already
 * resolved endpoint
 *
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
}

/**
 * Handle the sending and receiving procedures for a specific request message
 *
 * @param request_msg the message that has to be sent
 * @return an std::pair containing the following two values:
 * @return - a boost::logic::tribool value which indicates if the communication was successful or not
 * @return - std::optional containing a communication::message if the boost::logic::tribool is true, std::nullopt otherwise
 */
std::pair<boost::logic::tribool, std::optional<communication::message>> connection::sync_post(
        communication::message const &request_msg
) {
    std::pair<boost::logic::tribool, std::optional<communication::message>> result;
    result.first = this->write(request_msg);
    if (boost::indeterminate(result.first) || !result.first) return result;
    return this->read();
}

/**
 * Handle the sending and receiving procedures for a specific request message
 * using the internal thread. A provided callback will be executed
 * on completion
 *
 * @param request_msg the message that has to be sent
 * @param fn the callback that has to be executed on completion
 * @return void
 */
void connection::async_post(
        communication::message const &request_msg,
        std::function<void(std::optional<communication::message> const &)> const &fn
) {
    boost::asio::post(this->strand_, [this, request_msg, fn]() {
        std::pair<boost::logic::tribool, std::optional<communication::message>> result;
        result.first = this->write(request_msg);
        if (boost::indeterminate(result.first)) {
            this->handle_reconnection_();
        }
        if (boost::indeterminate(result.first) || !result.first) return fn(result.second);
        result = this->read();
        if (boost::indeterminate(result.first)) {
            this->handle_reconnection_();
        }
        if (boost::indeterminate(result.first) || !result.first) return fn(result.second);
    });
}

/**
 * Handle the sending and receiving procedures for a specific f_message
 * using the internal thread. The entire f_message is sent
 * through different sequential message to server.
 * A provided callback will be executed on completion
 *
 * @param request_msg f_message that has to be sent
 * @param fn the callback that has to be executed on completion
 * @return void
 */
void connection::async_post(
        std::shared_ptr<communication::f_message> const &request_msg,
        std::function<void(std::optional<communication::message> const &)> const &fn
) {
    boost::asio::post(this->strand_, [this, request_msg, fn]() {
        std::optional<communication::message> msg;
        try {
            std::pair<boost::logic::tribool, std::optional<communication::message>> result;
            while (request_msg->next_chunk()) {
                result.first = this->write(*request_msg);
                if (boost::indeterminate(result.first)) {
                    this->handle_reconnection_();
                }
                if (boost::indeterminate(result.first) || !result.first) return fn(result.second);
                result = this->read();
                if (boost::indeterminate(result.first)) {
                    this->handle_reconnection_();
                }
                if (boost::indeterminate(result.first) || !result.first) return fn(result.second);
            }
            fn(result.second);
        }
        catch (std::exception &e) {
            fn(std::nullopt);
        }
    });
}

/**
 * Allow to send a message to the server.
 *
 * @param request_msg message that has to be sent
 * @return true if the message has been successfully sent, false if there has been
 * an error and boost::indeterminate if the connection has been closed
 * // (true, std::optional<message>)
// (false, std::nullopt)
// (indeterminate, std::nullopt) // failed connection
 */
boost::logic::tribool connection::write(communication::message const &request_msg) {
    this->keepalive_timer_.cancel();
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
            std::cerr << "Connection to the server has been lost. Trying to reconnect..." << std::endl;
            return boost::indeterminate; // indicate failed connection
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
 * @return an std::pair containing the following two values:
 * @return - a boost::logic::tribool value which indicates if the message has been successfully read (true),
 * if the read operation has been failed (false) or if the connection has been closed (boost::indeterminate)
 * @return - std::optional containing a communication::message if the boost::logic::tribool is true, std::nullopt otherwise
 */
std::pair<boost::logic::tribool, std::optional<communication::message>> connection::read() {
    size_t header = 0;
    auto raw_msg_ptr = std::make_shared<std::vector<uint8_t>>();
    auto insert_pos = raw_msg_ptr->begin();
    bool ended = false;
    communication::MSG_TYPE msg_type;
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
        return {true, std::make_optional<communication::message>(raw_msg_ptr)};
    }
    catch (boost::system::system_error &ex) {
        auto error_code = ex.code();
        if (error_code == boost::asio::error::eof ||
            error_code == boost::asio::error::connection_reset ||
            error_code == boost::asio::error::broken_pipe) {
            std::cerr << "Connection to the server has been lost. Trying to reconnect..." << std::endl;
            return {boost::indeterminate, std::nullopt};
        }
        return {false, std::nullopt};
    }
    catch (std::exception &ex) {
        std::cerr << "Error in read():\n\t" << ex.what() << std::endl;
        this->schedule_keepalive();
        return {false, std::nullopt};
    }
}

/**
 * Allow to set the reconnection handler
 *
 * @param fn the handler for reconnection management
 * @return void
 */
void connection::set_reconnection_handler(std::function<void(void)> const &fn) {
    this->handle_reconnection_.connect(fn);
}

/**
 * Allow to join the connection thread.
 *
 * @return void
 */
void connection::join_thread() {
    if (this->thread_.joinable()) this->thread_.join();
}