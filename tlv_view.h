//
// Created by leonardo on 01/11/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_TLV_VIEW_H
#define REMOTE_BACKUP_M1_CLIENT_TLV_VIEW_H
#include <vector>
#include "op.h"

namespace operation {
    class tlv_view {
        operation::TLV_TYPE tlv_type_;
        size_t length_;
        std::shared_ptr<std::vector<uint8_t>> raw_op_;
        std::vector<uint8_t>::iterator begin_, end_;
        std::vector<uint8_t>::iterator data_begin_, data_end_;
        bool is_valid_;
        bool is_finished_;
    public:
        tlv_view(operation::op const & operation)
                : raw_op_ {operation.get_raw_op()}
                , begin_{raw_op_->begin()}
                , end_{raw_op_->end()}
                , data_begin_ {begin_ + 1}
                , data_end_ {begin_ + 1}
                , is_valid_{false}
                , is_finished_ {false} {}

        bool next_tlv() {
            if (this->is_finished_) return false;
            this->data_begin_ = this->data_end_;
            if (this->data_begin_ == this->end_) {
                this->is_finished_ = true;
                this->is_valid_ = false;
                return false;
            }
            this->tlv_type_ = static_cast<operation::TLV_TYPE>(*this->data_begin_++);
            this->length_ = 0;
            for (int i=0; i<4; i++) {
                this->length_ += *this->data_begin_++ << 8*(3-i);
            }
            this->data_end_ = this->data_begin_ + this->length_;
            this->is_valid_ = true;
            return true;
        }
        bool is_valid() {
            return this->is_valid_;
        }
        bool is_finished() {
            return this->is_finished_;
        }
        int get_tlv_type() const {
            return static_cast<int>(this->tlv_type_);
        }
        size_t get_length() {
            return this->length_;
        }
        std::vector<uint8_t>::iterator begin() {
            return this->data_begin_;
        }
        std::vector<uint8_t>::iterator end() {
            return this->data_end_;
        }
        std::vector<uint8_t>::const_iterator cbegin() const {
            return this->data_begin_;
        }
        std::vector<uint8_t>::const_iterator cend() const {
            return this->data_end_;
        }
    };
}


#endif //REMOTE_BACKUP_M1_CLIENT_TLV_VIEW_H
