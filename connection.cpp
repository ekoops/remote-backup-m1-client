//
// Created by leonardo on 16/11/20.
//

#include "connection.h"
#include "tlv_view.h"
#include "tools.h"
#include <boost/archive/binary_oarchive.hpp>
#include <algorithm>
#include <boost/thread.hpp>

connection::connection(boost::asio::io_context &io)
        : work_ {io}, strand_{boost::asio::make_strand(io)}, socket_{strand_}, resolver_{strand_},
          buffer_{std::make_shared<std::vector<uint8_t>>()} {
    for (int i=0; i<8; i++) {
        this->thread_group_.emplace_back(boost::bind(&boost::asio::io_context::run, &io), std::ref(io));
    }
}

void connection::connect(std::string const &hostname, std::string const &portno) {
    boost::asio::ip::tcp::resolver::results_type endpoints =
            this->resolver_.resolve(hostname, portno); // equivalente a gethostbyname
    boost::asio::connect(this->socket_, endpoints);
}

std::shared_ptr<connection>
connection::get_instance(boost::asio::io_context &io) {
    return std::shared_ptr<connection>(new connection{io});
}

bool connection::write(communication::message const &msg) {
    this->header_ = msg.get_raw_msg_ptr()->size();
    try {
        boost::asio::write(this->socket_, boost::asio::buffer(&this->header_, sizeof(this->header_)));
        this->buffer_->resize(11);
        boost::asio::read(this->socket_, boost::asio::buffer(*this->buffer_));
        communication::message ctrl_msg{this->buffer_};
        communication::tlv_view view{ctrl_msg};
        if (ctrl_msg.get_msg_type() == communication::MESSAGE_TYPE::CTRL &&
            view.next_tlv() &&
            view.get_tlv_type() == communication::TLV_TYPE::OK) {
            boost::asio::write(this->socket_, msg.buffer());
            return true;
        } else return false;
    }
    catch (std::exception &ex) {
        return false;
    }
}

std::optional<communication::message> connection::read() {
    this->header_ = 0;
    try {
        boost::asio::read(this->socket_, boost::asio::buffer(&this->header_, sizeof(this->header_)));
        communication::message ctrl_msg{communication::MESSAGE_TYPE::CTRL};
        ctrl_msg.add_TLV(communication::TLV_TYPE::OK);
        ctrl_msg.add_TLV(communication::TLV_TYPE::END);
        boost::asio::write(this->socket_, ctrl_msg.buffer());
        communication::message response_msg {this->header_};
        boost::asio::read(this->socket_, response_msg.buffer());
        return response_msg;
    }
    catch (std::exception &ex) {
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
        std::cerr << "Failed to read server control message" << std::endl;
        fail_callback();
    }
}

void connection::async_write(communication::message const &msg,
                             std::function<void(void)> const &success_callback,
                             std::function<void(void)> const &fail_callback) {
    size_t header = msg.get_raw_msg_ptr()->size();
    this->socket_.async_write_some(boost::asio::buffer(&header, sizeof(header)),
                                   boost::bind(&connection::handle_header_write,
                                               this,
                                               boost::asio::placeholders::error,
                                               msg,
                                               success_callback, fail_callback));
}

void connection::parse_payload(const boost::system::error_code &e,
                               std::function<void(communication::message const &)> const &success_callback,
                               std::function<void(void)> const &fail_callback) {
    if (!e) {
        communication::message msg{this->buffer_};
        success_callback(msg);
    } else {
        std::cerr << "Failed to read server message" << std::endl;
        fail_callback();
    }
}

void connection::handle_payload_read(const boost::system::error_code &e,
                                     std::function<void(communication::message const &)> const &success_callback,
                                     std::function<void(void)> const &fail_callback) {
    if (!e) {
        this->socket_.async_read_some(boost::asio::buffer(*this->buffer_),
                                      boost::bind(&connection::parse_payload, this,
                                                  boost::asio::placeholders::error,
                                                  success_callback, fail_callback));
    } else {
        std::cerr << "Failed to read server message" << std::endl;
        fail_callback();
    }
}

void connection::handle_header_read(const boost::system::error_code &e,
                                    std::function<void(communication::message const &)> const &success_callback,
                                    std::function<void(void)> const &fail_callback) {
    communication::message ctrl_msg{communication::MESSAGE_TYPE::CTRL};
    if (!e) {
        try {
            this->buffer_->resize(this->header_);
            ctrl_msg.add_TLV(communication::TLV_TYPE::OK);
            ctrl_msg.add_TLV(communication::TLV_TYPE::END);
            this->socket_.async_write_some(ctrl_msg.buffer(),
                                           boost::bind(&connection::handle_payload_read, this,
                                                       boost::asio::placeholders::error,
                                                       success_callback, fail_callback));
        }
        catch (std::exception &ex) {
            // Failed to realocate buffer, mainly  because the requested size is too much
            // Sending error message
            std::cerr << "Failed to resize buffer for server response" << std::endl;
//            ctrl_msg.add_TLV(communication::TLV_TYPE::ERROR);
//            ctrl_msg.add_TLV(communication::TLV_TYPE::END);
//            this->socket_.async_write_some(ctrl_msg.buffer(), boost::bind(std::plus<int>(), 0, 0));
//            fail_callback();
            std::exit(-1);
        }
    } else {
        std::cerr << "Failed to read server message" << std::endl;
        fail_callback();
    }
}

void connection::async_read(std::function<void(communication::message const &)> const &success_callback,
                            std::function<void(void)> const &fail_callback) {
    this->socket_.async_read_some(boost::asio::buffer(&this->header_, sizeof(this->header_)),
                                  boost::bind(&connection::handle_header_read, this,
                                              boost::asio::placeholders::error,
                                              success_callback, fail_callback));
}

