//
// Created by leonardo on 25/11/20.
//

#include "f_message.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/exception.hpp>

using namespace communication;

f_message::f_message(MESSAGE_TYPE msg_type,
                     boost::filesystem::path const &path,
                     std::string const& sign)
                     : message{msg_type}
                     , ifs_ {path, std::ios_base::binary} {
    this->ifs_.unsetf(std::ios::skipws);
    this->ifs_.seekg(0, std::ios::end);
    this->remaining_ = this->f_length_ = this->ifs_.tellg();
    this->ifs_.seekg(0, std::ios::beg);
    this->add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
    this->header_size_ = this->size();
    this->f_content_ = this->get_raw_msg_ptr()->end();
    this->resize(CHUNK_SIZE);
    this->f_content_[0] = communication::TLV_TYPE::CONTENT;
    std::advance(this->f_content_, 1);
}

std::shared_ptr<communication::f_message> f_message::get_instance(MESSAGE_TYPE msg_type,
                                                       boost::filesystem::path const &path,
                                                       std::string const &sign) {
    return std::shared_ptr<communication::f_message>(new f_message {msg_type, path, sign});
}

bool f_message::next_chunk() {
    if (this->remaining_ == 0) return false;
    size_t to_read;
    bool last = false;
    if (this->remaining_ >= CHUNK_SIZE - this->header_size_ - 5 - 5) {
        to_read = CHUNK_SIZE - this->header_size_ - 5;
    }
    else {
        to_read = this->remaining_;
        last = true;
    }

    for (int i = 0; i < 4; i++) {
        this->f_content_[i] = (to_read >> (3 - i) * 8) & 0xFF;
    }

    this->ifs_.read(reinterpret_cast<char *>(&*(this->f_content_+4)), to_read);
    if (!ifs_) {
        throw "Unexpected EOF";
    }

    if (last) {
        this->resize(this->header_size_ + 5 + this->remaining_);
        this->add_TLV(communication::TLV_TYPE::END);
        this->ifs_.close();
    }
    this->remaining_ -= to_read;
    return true;
}
