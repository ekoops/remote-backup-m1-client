#include <boost/functional/hash.hpp>
#include "message.h"
#include "tlv_view.h"


using namespace communication;

/**
 * Construct a message instance with a specific message type
 *
 * @param msg_type the message type
 * @return a new constructed message instance
 */
message::message(MESSAGE_TYPE msg_type) : raw_msg_ptr_{std::make_shared<std::vector<uint8_t>>()} {
    this->raw_msg_ptr_->push_back(static_cast<uint8_t>(msg_type));
}

/**
 * Construct a message instance from an std::shared_ptr to a buffer
 *
 * @param raw_msg_ptr a std::shared_ptr to a buffer
 * @return a new constructed message instance
 */
message::message(std::shared_ptr<std::vector<uint8_t>> raw_msg_ptr): raw_msg_ptr_ {std::move(raw_msg_ptr)} {}

/**
 * Allow to add a TLV segment to message
 *
 * @param tlv_type the TLV type
 * @param length the TLV value field length
 * @param buffer a pointer to the data that has to be copied in the TLV value field
 * @return void
 */
void message::add_TLV(TLV_TYPE tlv_type, size_t length, char const *buffer) {
    this->raw_msg_ptr_->reserve(this->raw_msg_ptr_->size() + 5 + length);
    this->raw_msg_ptr_->push_back(static_cast<uint8_t>(tlv_type));
    for (int i = 0; i < 4; i++) {
        this->raw_msg_ptr_->push_back((length >> (3 - i) * 8) & 0xFF);
    }
    for (int i = 0; i < length; i++) {
        this->raw_msg_ptr_->push_back(buffer[i]);
    }
}

/**
* Getter for the internal message representation pointer.
*
* @return a pointer to the internal message buffer
*/
std::shared_ptr<std::vector<uint8_t>> message::raw_msg_ptr() const {
    return this->raw_msg_ptr_;
}

/**
* Getter for the message type.
*
* @return the message type
*/
MESSAGE_TYPE message::msg_type() const {
    return static_cast<MESSAGE_TYPE>((*this->raw_msg_ptr_)[0]);
}

/**
* Allow to obtain a suitable buffer representation for sockets
*
* @return a suitable buffer representation for sockets
*/
boost::asio::mutable_buffer message::buffer() const {
    return boost::asio::buffer(*this->raw_msg_ptr_, this->size());
}

/**
* Allow to obtain the message buffer size
*
* @return the message buffer size
*/
size_t message::size() const {
    return this->raw_msg_ptr_->size();
}

/**
* Allow to resize the message buffer
*
* @param length the new message buffer size
* @return void
*/
void message::resize(size_t length) {
    this->raw_msg_ptr_->resize(length);
}

bool message::operator==(message const &other) const {
    return *this->raw_msg_ptr_ == *other.raw_msg_ptr_;
}

std::ostream &communication::operator<<(std::ostream &os, communication::message const &msg) {
    os << "Type: " << static_cast<int>(msg.msg_type()) << std::endl;
    communication::tlv_view view{msg};
    while (view.next_tlv()) {
        os << "\tT: " << static_cast<int>(view.tlv_type()) << std::endl;
        os << "\tL: " << view.length() << std::endl;
        if (view.tlv_type() != communication::TLV_TYPE::CONTENT) {
            os << "\tV: " << std::string(view.cbegin(), view.cend()) << std::endl;
        }
    }
    return os;
}