//
// Created by leonardo on 16/11/20.
//

#include "connection.h"
#include "tlv_view.h"
#include "tools.h"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <algorithm>

connection::connection(boost::asio::io_context &io) : socket_{io}, resolver_{io} {}

void connection::connect(std::string const &hostname, std::string const &portno) {
    boost::asio::ip::tcp::resolver::results_type endpoints =
            this->resolver_.resolve(hostname, portno); // equivalente a gethostbyname
    boost::asio::connect(this->socket_, endpoints);
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
//    boost::asio::write(this->socket_, boost::asio::buffer(*msg.get_raw_msg_ptr()));

//    boost::asio::streambuf buf;
//    std::ostream os{&buf};
//    boost::archive::binary_oarchive boar{os};
//    boar & msg;

//    size_t length = msg.get_raw_msg_ptr()->size();
//    std::vector<uint8_t> header(8);
//    for (int i = 0; i < 8; i++) {
//        header[i] = length >> (7 - i) * 8 & 0xFF;
//    }
    size_t header = msg.get_raw_msg_ptr()->size();
    std::vector<boost::asio::const_buffer> buffers;
    buffers.emplace_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.emplace_back(boost::asio::buffer(*msg.get_raw_msg_ptr()));
    boost::asio::write(this->socket_, buffers);
}
//
//communication::message connection::read() {
////    std::vector<uint8_t> header(8);
////    boost::asio::read(this->socket_, boost::asio::buffer(header, 7));
////    communication::message msg;
////    size_t length = 0;
////    for (int i = 0; i < 8; i++) {
////        length += header[i] << (7-i)*8;
////    }
////    std::cout << length << std::endl;
////    msg.reserve(length);
////    boost::asio::read(this->socket_, boost::asio::buffer(*msg.get_raw_msg_ptr()));
////    std::for_each(msg.get_raw_msg_ptr()->begin(), msg.get_raw_msg_ptr()->end(), [](uint8_t a) {std::cout << std::to_string(a) << std::endl;});
//
////    return msg;
//
////    // read body
////    boost::asio::streambuf buf;
////    boost::asio::read(this->socket_, buf.prepare(header));
////    buf.commit(header);
////
////    // deserialize
////    std::istream is(&buf);
////    boost::archive::binary_iarchive biar(is);
////    communication::message msg;
////    biar & msg;
////    return msg;
//}

//void connection::write(communication::message const &msg) {
//    boost::asio::streambuf buf;
//    std::ostream os(&buf);
//    boost::archive::binary_oarchive boar{os};
//    boar & msg;
//    const size_t header = buf.size();
//
//    // send header and buffer using scatter
//    std::vector<boost::asio::const_buffer> buffers;
//    buffers.push_back(boost::asio::buffer(&header, sizeof(header)));
//    buffers.push_back(buf.data());
//    const size_t rc = boost::asio::write(
//            this->socket_,
//            buffers
//    );
//}

communication::message connection::read() {
    size_t header;
    boost::asio::read(this->socket_, boost::asio::buffer(&header, sizeof(header)));
    communication::message msg{header};

    boost::asio::read(this->socket_, msg.buffer());
    return msg;
}