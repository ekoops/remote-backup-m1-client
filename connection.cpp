//
// Created by leonardo on 16/11/20.
//

#include "connection.h"
#include "tlv_view.h"
#include "tools.h"
#include <boost/archive/binary_oarchive.hpp>
#include <algorithm>
#include <boost/thread.hpp>

namespace ssl = boost::asio::ssl;

connection::connection(boost::asio::io_context &io, ssl::context& ctx)
        : work_{io}, strand_{boost::asio::make_strand(io)}, ssl_socket_{strand_, ctx}, socket_{strand_},
          resolver_{strand_},
          buffer_{std::make_shared<std::vector<uint8_t >>(100)} {
    this->ssl_socket_.set_verify_mode(ssl::verify_peer);
    // TODO: modificare
    this->ssl_socket_.set_verify_callback(boost::bind(&connection::verify_certificate, this,
                                                      boost::placeholders::_1, boost::placeholders::_2));

    for (int i = 0; i < 8; i++) {
        this->thread_group_.emplace_back(boost::bind(&boost::asio::io_context::run, &io), std::ref(io));
    }
}

void connection::connect(std::string const &hostname, std::string &portno) {
    // resolve throws boost::system::system_error
    boost::asio::ip::tcp::resolver::results_type endpoints =
            this->resolver_.resolve(hostname, portno); // equivalente a gethostbyname

    // connect throws boost::system::system_error
        boost::asio::connect(this->socket_, endpoints);
//    boost::asio::connect(this->ssl_socket_.lowest_layer(), endpoints);
//    this->ssl_socket_.handshake(ssl::stream<boost::asio::ip::tcp::socket>::client);
}

std::shared_ptr<connection>
connection::get_instance(boost::asio::io_context &io, ssl::context const& ctx) {
    return std::shared_ptr<connection>(new connection{io, ctx});
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

bool connection::write(communication::message const &msg) {
    std::cout << "write1" << std::endl;
    size_t header = msg.get_raw_msg_ptr()->size();
    std::vector<boost::asio::mutable_buffer> buffers;
    buffers.emplace_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.emplace_back(boost::asio::buffer(msg.buffer()));
    try {
        std::cout << "write2" << std::endl;
        this->socket_.write_some(buffers);
        std::cout << "write3" << std::endl;
        return true;
    }
    catch (std::exception &ex) {
        std::cout << "write4" << std::endl;
        return false;
    }

}

std::optional<communication::message> connection::read() {
    std::cout << "read1" << std::endl;
    size_t header = 0;
    std::vector<boost::asio::mutable_buffer> buffers;
    buffers.emplace_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.emplace_back(boost::asio::buffer(*this->buffer_));
    try {
        std::cout << "read2" << std::endl;
        this->socket_.read_some(buffers);
        std::cout << "read3" << std::endl;
        auto raw_msg = std::make_shared<std::vector<uint8_t>>(this->buffer_->cbegin(),
                                                              std::next(this->buffer_->cbegin(), header));
        return communication::message{raw_msg};
    }
    catch (std::exception &ex) {
        std::cout << "read4" << std::endl;
        return {};
    }
}

void connection::handle_payload_write(const boost::system::error_code &e,
                                      communication::message const &msg,
                                      std::function<void(void)> const &success_callback,
                                      std::function<void(void)> const &fail_callback) {
    if (!e) {
        communication::message ctrl_msg{this->buffer_};
        communication::tlv_view view{ctrl_msg};
        if (view.next_tlv() && view.get_tlv_type() == communication::TLV_TYPE::OK) {
            this->socket_.async_write_some(msg.buffer(), boost::bind(success_callback));
        } else {
            std::cerr << "Server failed to resize buffer for client request" << std::endl;
            std::exit(-1);
        }
    } else {
        std::cerr << "e.message() " << e.message() << std::endl;
        std::cerr << "Failed to read server control message" << std::endl;
        fail_callback();
    }
}

void connection::handle_header_write(const boost::system::error_code &e,
                                     communication::message const &msg,
                                     std::function<void(void)> const &success_callback,
                                     std::function<void(void)> const &fail_callback) {

    if (!e) {
        try {
            this->buffer_->resize(11); // sizeof(CTRL MESSAGE) CTRL OK 0000 END 0000
            this->socket_.async_read_some(boost::asio::buffer(*this->buffer_),
                                          boost::bind(&connection::handle_payload_write, this,
                                                      boost::asio::placeholders::error,
                                                      msg,
                                                      success_callback, fail_callback));
        }
        catch (std::exception &ex) {
            std::cerr << "Failed to resize buffer for server response" << std::endl;
            std::exit(-1);
        }
    } else {
        std::cerr << "e.message() " << e.message() << std::endl;
        std::cerr << "Failed to read server control message" << std::endl;
        fail_callback();
    }
}

void connection::async_write(communication::message const &msg,
                             std::function<void(void)> const &success_callback,
                             std::function<void(void)> const &fail_callback) {
    std::unique_lock ul{this->m_};
    this->header_ = msg.get_raw_msg_ptr()->size();
    this->socket_.async_write_some(boost::asio::buffer(&this->header_, sizeof(this->header_)),
                                   boost::bind(&connection::handle_header_write,
                                               this,
                                               boost::asio::placeholders::error,
                                               msg,
                                               success_callback, fail_callback));
}


void connection::execute_callback(boost::system::error_code const &e,
                                  std::function<void(communication::message const &)> const &fn) {
    std::unique_lock ul{this->m_};
    std::cout << "START EXECUTE CALLBACK" << std::endl;

    if (e) {
        std::cout << "ERROR" << std::endl;
        return;
    }
    auto ptr = std::make_shared<std::vector<uint8_t>>(this->buffer_->cbegin(),
                                                      std::next(this->buffer_->cbegin(), this->read_header_));
    communication::message response{ptr};
    fn(response);
    this->can_write = true;
    this->cv_.notify_all();
    std::cout << "END EXECUTE CALLBACK" << std::endl;
    //        auto ptr = std::make_shared<std::vector<uint8_t>>();
//    for (int i = 0; i < this->header_; i++) {
//        ptr->push_back(std::move((*this->buffer_)[i]));
//    }
//    std::cout << communication::message {ptr};
}

void connection::read_response(boost::system::error_code const &e,
                               std::function<void(communication::message const &)> const &fn) {

    std::cout << "START READ RESPONSE" << std::endl;
    if (!e) {
        std::vector<boost::asio::mutable_buffer> buffers;
        buffers.emplace_back(boost::asio::buffer(&this->read_header_, sizeof(this->read_header_)));
        buffers.emplace_back(boost::asio::buffer(*this->buffer_));
        this->socket_.async_read_some(buffers,
                                      boost::bind(&connection::execute_callback, this, boost::asio::placeholders::error,
                                                  fn));
    } else std::cout << "ERROR" << std::endl;
    std::cout << "END READ RESPONSE" << std::endl;
}

void connection::async_write2(communication::message const &msg,
                              std::function<void(communication::message const &)> const &fn) {
    std::unique_lock ul{this->m_};
    while (!this->can_write) this->cv_.wait(ul);
    this->can_write = false;
    std::cout << "START ASYNC WRITE" << std::endl;
    std::vector<boost::asio::mutable_buffer> buffers;
    this->header_ = msg.get_raw_msg_ptr()->size();
    std::cout << "HEADER: " << this->header_ << std::endl;
    buffers.emplace_back(boost::asio::buffer(&this->header_, sizeof(this->header_)));
    buffers.emplace_back(msg.buffer());
    this->socket_.async_write_some(buffers,
                                   boost::bind(&connection::read_response, this, boost::asio::placeholders::error, fn));
    std::cout << "END ASYNC WRITE" << std::endl;
}

void connection::parse_payload(const boost::system::error_code &e,
                               std::function<void(communication::message const &)> const &success_callback,
                               std::function<void(void)> const &fail_callback
) {
    if (!e) {
        communication::message msg{this->buffer_};
        success_callback(msg);
    } else {
        std::cerr << "e.message() " << e.

                message()

                  <<
                  std::endl;
        std::cerr << "Failed to read server message" <<
                  std::endl;

        fail_callback();

    }
}

void connection::handle_payload_read(const boost::system::error_code &e,

                                     std::function<
                                             void(communication::message
                                                  const &)> const &success_callback,
                                     std::function<void(void)> const &fail_callback
) {
    if (!e) {
        this->socket_.
                async_read_some(boost::asio::buffer(*this->buffer_),
                                boost::bind(&connection::parse_payload, this,
                                            boost::asio::placeholders::error,
                                            success_callback, fail_callback)
        );
    } else {
        std::cerr << "e.message() " << e.

                message()

                  <<
                  std::endl;
        std::cerr << "Failed to read server message" <<
                  std::endl;

        fail_callback();

    }
}

void connection::handle_header_read(const boost::system::error_code &e,

                                    std::function<
                                            void(communication::message
                                                 const &)> const &success_callback,
                                    std::function<void(void)> const &fail_callback
) {
    communication::message ctrl_msg{communication::MESSAGE_TYPE::CTRL};
    if (!e) {
        try {
            this->buffer_->resize(this->header_);
            ctrl_msg.
                    add_TLV(communication::TLV_TYPE::OK);
            ctrl_msg.
                    add_TLV(communication::TLV_TYPE::END);
            this->socket_.
                    async_write_some(ctrl_msg.buffer(),
                                     boost::bind(&connection::handle_payload_read, this,
                                                 boost::asio::placeholders::error,
                                                 success_callback, fail_callback)

            );
        }
        catch (std::exception &ex) {
// Failed to realocate buffer, mainly  because the requested size is too much
// Sending error message
            std::cerr << "Failed to resize buffer for server response" <<
                      std::endl;
//            ctrl_msg.add_TLV(communication::TLV_TYPE::ERROR);
//            ctrl_msg.add_TLV(communication::TLV_TYPE::END);
//            this->socket_.async_write_some(ctrl_msg.buffer(), boost::bind(std::plus<int>(), 0, 0));
//            fail_callback();
            std::exit(-1);
        }
    } else {
        std::cerr << "e.message() " << e.

                message()

                  <<
                  std::endl;
        std::cerr << "Failed to read server message" <<
                  std::endl;

        fail_callback();

    }
}

void connection::async_read(std::function<void(communication::message
                                               const &)> const &success_callback,
                            std::function<void(void)> const &fail_callback
) {
    this->socket_.
            async_read_some(boost::asio::buffer(&this->header_, sizeof(this->header_)),
                            boost::bind(&connection::handle_header_read, this,
                                        boost::asio::placeholders::error,
                                        success_callback, fail_callback)
    );
}

