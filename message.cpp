//
// Created by leonardo on 30/10/20.
//

#include "message.h"
#include "tlv_view.h"
#include <boost/functional/hash.hpp>


using namespace communication;

message::message(MESSAGE_TYPE msg_type) : raw_msg_{std::make_shared<std::vector<uint8_t>>()} {
    this->raw_msg_->push_back(static_cast<uint8_t>(msg_type));
}

message::message(size_t length) : raw_msg_{std::make_shared<std::vector<uint8_t>>(length)} {}

message::message(MESSAGE_TYPE msg_type, size_t length): raw_msg_{std::make_shared<std::vector<uint8_t>>(length)} {
    (*this->raw_msg_)[0] = msg_type;
}
message::message(std::shared_ptr<std::vector<uint8_t>> raw_msg): raw_msg_ {std::move(raw_msg)} {}

void message::add_TLV(TLV_TYPE tlv_type, size_t length, char const *buffer) {
    this->raw_msg_->reserve(this->raw_msg_->size() + 5 + length);
    this->raw_msg_->push_back(static_cast<uint8_t>(tlv_type));
    for (int i = 0; i < 4; i++) {
        this->raw_msg_->push_back((length >> (3 - i) * 8) & 0xFF);
    }
    for (int i = 0; i < length; i++) {
        this->raw_msg_->push_back(buffer[i]);
    }
}


void message::add_TLV(TLV_TYPE tlv_type, boost::filesystem::path const &path) {
    boost::filesystem::ifstream ifs;
    ifs.open(path, std::ios_base::binary);
    ifs.unsetf(std::ios::skipws);           // Stop eating new lines in binary mode!!!
    std::streampos length;
    ifs.seekg(0, std::ios::end);
    length = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    this->raw_msg_->reserve(this->raw_msg_->size() + 5 + length);
    this->raw_msg_->push_back(static_cast<uint8_t>(tlv_type));
    for (int i = 0; i < 4; i++) {
        this->raw_msg_->push_back((length >> (3 - i) * 8) & 0xFF);
    }
    this->raw_msg_->insert(this->raw_msg_->end(),
                           std::istream_iterator<uint8_t>(ifs), std::istream_iterator<uint8_t>());
}



std::shared_ptr<std::vector<uint8_t>> message::get_raw_msg_ptr() const {
    return this->raw_msg_;
}

MESSAGE_TYPE message::get_msg_type() const {
    return static_cast<MESSAGE_TYPE>((*this->raw_msg_)[0]);
}

boost::asio::mutable_buffer message::buffer() const {
    return boost::asio::buffer(*this->raw_msg_);
}

size_t message::size() const {
    return this->raw_msg_->size();
}

void message::resize(size_t length) {
    this->raw_msg_->resize(length);
}

bool message::operator==(message const &other) const {
    return *this->raw_msg_ == *other.raw_msg_;
}

std::ostream &communication::operator<<(std::ostream &os, communication::message const &msg) {
    os << "Type: " << static_cast<int>(msg.get_msg_type()) << std::endl;
    communication::tlv_view view{msg};
    while (view.next_tlv()) {
        os << "\tT: " << static_cast<int>(view.get_tlv_type()) << std::endl;
        os << "\tL: " << view.get_length() << std::endl;
        if (view.get_tlv_type() != communication::TLV_TYPE::CONTENT) {
            os << "\tV: " << std::string(view.cbegin(), view.cend()) << std::endl;
        }
    }
    return os;
}

//std::size_t communication::hash_value(communication::message const &msg) {
//    boost::hash<std::vector<uint8_t>> hasher;
//    return hasher(*msg.get_raw_msg_ptr());
//}
//
//
//template<>
//struct std::equal_to<message> {
//    bool operator()(communication::message &msg1, communication::message &msg2) {
//        return msg1.get_raw_msg_ptr() == msg2.get_raw_msg_ptr();
//    }
//};


