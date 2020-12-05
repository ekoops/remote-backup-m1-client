#include <boost/archive/binary_oarchive.hpp>
#include <boost/thread.hpp>
#include <algorithm>
#include "connection.h"
#include "f_message.h"

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
    resolver_{strand_} {
    this->socket_.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);
    this->socket_.set_verify_callback(boost::bind(&connection::verify_certificate, this,
                                                  boost::placeholders::_1, boost::placeholders::_2));
    this->thread_pool_.reserve(thread_pool_size);
    for (int i = 0; i < thread_pool_size; i++) {
        this->thread_pool_.emplace_back(boost::bind(&boost::asio::io_context::run, &io), std::ref(io));
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

bool connection::verify_certificate(bool preverified, ssl::verify_context &ctx) {
    // TODO: Viene chiamata due volte... Perchè?
    std::cout << "preverified: " << preverified << std::endl;
    return true;
}

/**
 * Allow to establish an SSL socket connection with the specified endpoint
 *
 * @param hostname the desired endpoint hostname
 * @param service the desired endpoint service/port number
 * @return void
 */
void connection::connect(std::string const &hostname, std::string const &service) {
    try {
        boost::asio::ip::tcp::resolver::results_type endpoints =
                this->resolver_.resolve(hostname, service);
        boost::asio::connect(this->socket_.lowest_layer(), endpoints);
        this->socket_.handshake(ssl::stream<boost::asio::ip::tcp::socket>::client);
    }
    catch (boost::system::system_error &ex) {
        std::cerr << "Failed to access to service " << service
                  << " from " << hostname << ":\n\t" << ex.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    catch (std::exception &ex) {
        std::cerr << "Error in connect():\n\t" << ex.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
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
        this->write(request_msg);
        fn(this->read());
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
    size_t header = request_msg.raw_msg_ptr()->size();
    std::vector<boost::asio::mutable_buffer> buffers;
    buffers.emplace_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.emplace_back(request_msg.buffer());
    try {
        std::cout << "<<<<<<<<<<REQUEST>>>>>>>>>" << std::endl;
        std::cout << "HEADER: " << header << std::endl;
        std::cout << request_msg;
        boost::asio::write(this->socket_, buffers);
        return true;
    }
    catch (boost::system::system_error &ex) {
        auto error_code = ex.code();
        if (error_code == boost::asio::error::eof ||
            error_code == boost::asio::error::connection_reset) {
            std::cerr << "Connection to the server has been lost" << std::endl;
            std::exit(EXIT_FAILURE);
        } else {
            std::cerr << "Failed to write message to server" << std::endl;
            return false;
        }
    }
    catch (std::exception &ex) {
        std::cerr << "Error in write():\n\t" << ex.what() << std::endl;
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
    try {
        boost::asio::read(this->socket_, boost::asio::buffer(&header, sizeof(header)));
        auto raw_msg_ptr = std::make_shared<std::vector<uint8_t>>(header);
        boost::asio::read(this->socket_, boost::asio::buffer(*raw_msg_ptr, header));
        communication::message response_msg{raw_msg_ptr};
        std::cout << "<<<<<<<<<<RESPONSE>>>>>>>>>" << std::endl;
        std::cout << "HEADER: " << header << std::endl;
        std::cout << response_msg;
        return std::make_optional<communication::message>(response_msg);
    }
    catch (boost::system::system_error &ex) {
        auto error_code = ex.code();
        if (error_code == boost::asio::error::eof ||
            error_code == boost::asio::error::connection_reset) {
            std::cerr << "Connection to the server has been lost" << std::endl;
            std::exit(EXIT_FAILURE);
        } else {
            std::cerr << "Failed to read message from server" << std::endl;
            return std::nullopt;
        }
    }
    catch (std::exception &ex) {
        std::cerr << "Error in read():\n\t" << ex.what() << std::endl;
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