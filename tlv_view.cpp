#include "tlv_view.h"

using namespace communication;

/**
 * Construct a tlv_view instance for a specific message
 *
 * @param msg the message on which the constructed view should operate
 * @return a new constructed tlv_view instance
 */
tlv_view::tlv_view(message const &msg)
        : raw_msg_ptr_{msg.raw_msg_ptr()}, begin_{raw_msg_ptr_->begin()}, end_{raw_msg_ptr_->end()},
          data_begin_{begin_ + 1}, data_end_{begin_ + 1}, valid_{false}, finished_{false} {}

/**
* Allow to shift the tlv_view on the next TLV.
*
* @return true if the tlv_view is valid and set on the next TLV, false if
* the message doesn't contain other TLV
*/
bool tlv_view::next_tlv() {
    if (this->finished_) return false;
    this->data_begin_ = this->data_end_;
    if (this->data_begin_ == this->end_) {
        this->finished_ = true;
        this->valid_ = false;
        return false;
    }
    this->tlv_type_ = static_cast<TLV_TYPE>(*this->data_begin_++);
    this->length_ = 0;
    for (int i = 0; i < 4; i++) {
        this->length_ += *this->data_begin_++ << 8 * (3 - i);
    }
    this->data_end_ = this->data_begin_ + this->length_;
    this->valid_ = true;
    return true;
}

/**
* Allow to check if the tlv_view is valid.
*
* @return true if the tlv_view is valid, false otherwise
*/
[[nodiscard]] bool tlv_view::valid() const {
    return this->valid_;
}

/**
* Allow to check if the tlv_view is finished (a tlv_view is finished
* if there are no other TLV segment to show on message).
*
* @return true if the tlv_view is valid, false otherwise
*/
[[nodiscard]] bool tlv_view::finished() const {
    return this->finished_;
}

/**
* Getter for the actual TLV segment type
*
* @return the actual TLV segment type
*/
[[nodiscard]] communication::TLV_TYPE tlv_view::tlv_type() const {
    if (!this->valid_) throw std::logic_error{"the tlv_view is not valid"};
    return this->tlv_type_;
}

/**
* Getter for the actual TLV segment value length
*
* @return the actual TLV segment value length
*/
[[nodiscard]] size_t tlv_view::length() const {
    if (!this->valid_) throw std::logic_error{"the tlv_view is not valid"};
    return this->length_;
}

std::vector<uint8_t>::iterator tlv_view::begin() {
    if (!this->valid_) throw std::logic_error{"the tlv_view is not valid"};
    return this->data_begin_;
}

std::vector<uint8_t>::iterator tlv_view::end() {
    if (!this->valid_) throw std::logic_error{"the tlv_view is not valid"};
    return this->data_end_;
}

[[nodiscard]] std::vector<uint8_t>::const_iterator tlv_view::cbegin() const {
    if (!this->valid_) throw std::logic_error{"the tlv_view is not valid"};
    return this->data_begin_;
}

std::vector<uint8_t>::const_iterator tlv_view::cend() const {
    if (!this->valid_) throw std::logic_error{"the tlv_view is not valid"};
    return this->data_end_;
}