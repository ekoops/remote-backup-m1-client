#include "connection.h"
#include <boost/archive/binary_oarchive.hpp>
#include <algorithm>
#include <boost/thread.hpp>
#include "f_message.h"

namespace ssl = boost::asio::ssl;

connection::connection(boost::asio::io_context &io,
                       ssl::context &ctx,
                       size_t thread_pool_size
) : work_{io}, strand_{boost::asio::make_strand(io)},
    socket_{strand_, ctx},
    resolver_{strand_} {

    this->socket_.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);
    this->socket_.set_verify_callback(boost::bind(&connection::verify_certificate, this,
                                                  boost::placeholders::_1, boost::placeholders::_2));
    this->thread_pool_.reserve(8);
    for (int i = 0; i < thread_pool_size; i++) {
        this->thread_pool_.emplace_back(boost::bind(&boost::asio::io_context::run, &io), std::ref(io));
    }
}

bool connection::verify_certificate(bool preverified, ssl::verify_context &ctx) {
    return true; // TODO
}

void connection::connect(std::string const &hostname, std::string const &service) {
    // resolve throws boost::system::system_error
    boost::asio::ip::tcp::resolver::results_type endpoints =
            this->resolver_.resolve(hostname, service); // equivalente a gethostbyname

    // connect throws boost::system::system_error
    try {
        boost::asio::connect(this->socket_.lowest_layer(), endpoints);
        this->socket_.handshake(ssl::stream<boost::asio::ip::tcp::socket>::client);
    }
    catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}

void connection::join_threads() {
    for (auto &t : this->thread_pool_) t.join();
}

std::shared_ptr<connection> connection::get_instance(boost::asio::io_context &io,
                                                     ssl::context &ctx,
                                                     size_t thread_pool_size) {
    return std::shared_ptr<connection>(new connection{io, ctx, thread_pool_size});
}

void connection::post(communication::message const &request_msg,
                      std::function<void(communication::message const &)> const &success_fn,
                      std::function<void(void)> const &fail_fn
) {
    boost::asio::post(this->strand_, [this, request_msg, success_fn, fail_fn]() {
        std::optional<communication::message> msg;
        if (this->write(request_msg) && (msg = this->read())) {
            success_fn(msg.value());
        } else fail_fn();
    });
}

void connection::post(const std::shared_ptr<communication::f_message> &request_msg,
                      std::function<void(communication::message const &)> const &success_fn,
                      std::function<void(void)> const &fail_fn) {
    boost::asio::post(this->strand_, [this, request_msg, success_fn, fail_fn]() {
        std::optional<communication::message> msg;
        try {
            while (request_msg->next_chunk()) {
                if (!this->write(*request_msg) || !(msg = this->read())) {
                    fail_fn();
                }
            }
            success_fn(msg.value());
        }
        catch (std::exception &e) {
            fail_fn();
        }
    });
}


bool connection::write(communication::message const &request_msg) {
    std::cout << "START WRITE" << std::endl;
    size_t header = request_msg.get_raw_msg_ptr()->size();
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
    catch (std::exception &ex) {
        std::cout << "WRITE EXCEPTION" << std::endl;
        return false;
    }
}

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
        return response_msg;
    }
    catch (std::exception &ex) {
        std::cout << "READ EXCEPTION" << std::endl;
        return {};
    }
}