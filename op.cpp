//
// Created by leonardo on 30/10/20.
//

#include "op.h"

using namespace operation;

op::op(OPERATION_TYPE op_type): raw_op_ {std::make_shared<std::vector<uint8_t>>()} {
    this->raw_op_->push_back(static_cast<uint8_t>(op_type));
}

void op::add_TLV(TLV_TYPE tlv_type, size_t length, char const *buffer) {
    this->raw_op_->reserve(this->raw_op_->size() + 5 + length);
    this->raw_op_->push_back(static_cast<uint8_t>(tlv_type));
    for (int i = 0; i < 4; i++) {
        this->raw_op_->push_back((length >> (3 - i) * 8) & 0xFF);
    }
    for (int i = 0; i < length; i++) {
        this->raw_op_->push_back(buffer[i]);
    }
}

void op::write_on_socket(boost::asio::ip::tcp::socket& socket) {
    boost::system::error_code error;
    auto buffer = *this->raw_op_;
    size_t len = socket.write_some(boost::asio::buffer(buffer), error);
    if (error || buffer.size() != len) {
        throw boost::system::system_error(error ? error : boost::asio::error::no_buffer_space);
    }
}
op op::read_from_socket(boost::asio::ip::tcp::socket& socket) {
    boost::system::error_code error;
    op operation;
    size_t len = socket.read_some(boost::asio::buffer(*operation.raw_op_), error);
    if (error) throw boost::system::system_error(error);
    else return operation;
}

std::shared_ptr<std::vector<uint8_t>> op::get_raw_op() const {
    return this->raw_op_;
}

OPERATION_TYPE op::get_op_type() const {
    return static_cast<OPERATION_TYPE>((*this->raw_op_)[0]);
}

bool op::operator==(op const &other) const {
    return this->raw_op_ == other.raw_op_;
}

