//
// Created by leonardo on 16/11/20.
//

#include "connection.h"
#include "tlv_view.h"
#include "tools.h"
#include <boost/archive/binary_oarchive.hpp>
#include <algorithm>

connection::connection(boost::asio::io_context &io)
        : strand_{boost::asio::make_strand(io)}, socket_{strand_}, resolver_{strand_}{}

void connection::connect(std::string const &hostname, std::string const &portno) {
    boost::asio::ip::tcp::resolver::results_type endpoints =
            this->resolver_.resolve(hostname, portno); // equivalente a gethostbyname
    boost::asio::connect(this->socket_, endpoints);
}

std::shared_ptr<connection>
connection::get_instance(boost::asio::io_context &io) {
    return std::shared_ptr<connection>(new connection{io});
}

bool connection::authenticate() {
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
        communication::message auth_msg{communication::MESSAGE_TYPE::AUTH};
        auth_msg.add_TLV(communication::TLV_TYPE::USRN, username.size(), username.c_str());
        auth_msg.add_TLV(communication::TLV_TYPE::PSWD, password.size(), password.c_str());
        auth_msg.add_TLV(communication::TLV_TYPE::END);
        tools::retry(boost::bind(&connection::write, this, auth_msg));
        communication::message msg = this->read();
        communication::tlv_view view{msg};
        if (view.next_tlv() && view.get_tlv_type() == communication::TLV_TYPE::OK) return true;
        else std::cout << "Authentication failed (attempts left " << --general_attempts << ")." << std::endl;
    }
    return false;
}

void connection::write(communication::message const &msg) {
    size_t header = msg.get_raw_msg_ptr()->size();
    std::vector<boost::asio::const_buffer> buffers;
    buffers.emplace_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.emplace_back(boost::asio::buffer(*msg.get_raw_msg_ptr()));
    boost::asio::write(this->socket_, buffers);
}

communication::message connection::read() {
    size_t header;
    boost::asio::read(this->socket_, boost::asio::buffer(&header, sizeof(header)));
    communication::message msg{header};

    boost::asio::read(this->socket_, msg.buffer());
    return msg;
}

void connection::handle_payload_write(const boost::system::error_code &e,
                                      communication::message const &msg,
                                      std::function<void(void)> const &callback) {
    if (!e) {
        communication::message msg{this->buffer_};
        communication::tlv_view view{msg};
        if (view.next_tlv() && view.get_tlv_type() == communication::TLV_TYPE::OK) {
            this->socket_.async_write_some(msg.buffer(), boost::bind(callback));
        }
    } else std::cerr << "Failed to read server control message" << std::endl;
}

void connection::handle_header_write(const boost::system::error_code &e,
                                     communication::message const &msg,
                                     std::function<void(void)> const &callback) {
    if (!e) {
        this->socket_.async_read_some(boost::asio::buffer(*this->buffer_),
                                      boost::bind(&connection::handle_payload_write, this,
                                                  boost::asio::placeholders::error,
                                                  msg,
                                                  callback));

    } else std::cerr << "Failed to write message" << std::endl;
}

void connection::async_write(communication::message const &msg,
                             std::function<void(void)> const &callback) {
    size_t header = msg.get_raw_msg_ptr()->size();
    this->socket_.async_write_some(boost::asio::buffer(&header, sizeof(header)),
                                   boost::bind(&connection::handle_header_write,
                                               this,
                                               boost::asio::placeholders::error,
                                               msg,
                                               callback));
}

void connection::parse_payload(const boost::system::error_code &e,
                               std::function<void(communication::message const&)> const &callback) {
    if (!e) {
        communication::message msg{this->buffer_};
        callback(msg);
    }
}

void connection::handle_payload_read(const boost::system::error_code &e,
                                     std::function<void(communication::message const&)> const &callback) {
    if (!e) {
        this->socket_.async_read_some(boost::asio::buffer(*this->buffer_),
                                      boost::bind(&connection::parse_payload, this,
                                                  boost::asio::placeholders::error,
                                                  callback));
    } else std::cerr << "Failed to read server message" << std::endl;
}

void connection::handle_header_read(const boost::system::error_code &e,
                                    std::function<void(communication::message const&)> const &callback) {
    communication::message ctrl_msg{communication::MESSAGE_TYPE::CTRL};
    if (!e) {
        try {
            this->buffer_->resize(this->header_);
            ctrl_msg.add_TLV(communication::TLV_TYPE::OK);
            ctrl_msg.add_TLV(communication::TLV_TYPE::END);
            this->socket_.async_write_some(ctrl_msg.buffer(),
                                           boost::bind(&connection::handle_payload_read, this,
                                                       boost::asio::placeholders::error,
                                                       callback));
        }
        catch (std::exception &ex) {
            // Failed to realocate buffer, mainly  because the requested size is too much
            // Sending error message
            std::cerr << "Failed to resize buffer for server response" << std::endl;
            std::exit(-1);
        }
    } else std::cerr << "Failed to read server message" << std::endl;
}

void connection::async_read(std::function<void(communication::message const&)> const &callback) {
    this->socket_.async_read_some(boost::asio::buffer(&this->header_, sizeof(this->header_)),
                                  boost::bind(&connection::handle_header_read, this,
                                              boost::asio::placeholders::error,
                                              callback));
}

