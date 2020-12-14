#include "f_message.h"
#include <boost/filesystem/exception.hpp>

using namespace communication;
namespace fs = boost::filesystem;

size_t const f_message::CHUNK_SIZE = 4096;

/**
 * Construct an f_message instance for a specific file.
 *
 * @param msg_type the message type
 * @param path the file absolute path
 * @param path the file sign
 * @return a new constructed f_message instance
 */
f_message::f_message(
        MSG_TYPE msg_type,
        fs::path const &path,
        std::string const &sign
) // the sign is added to improve performance
        : message{msg_type}, ifs_{path, std::ios_base::binary}, completed_{false} {
    this->ifs_.unsetf(std::ios::skipws);
    this->ifs_.seekg(0, std::ios::end);
    this->remaining_ = this->ifs_.tellg();
    this->ifs_.seekg(0, std::ios::beg);
    this->add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
    this->header_size_ = this->size();
    this->resize(CHUNK_SIZE);
    this->f_content_ = std::next(this->raw_msg_ptr()->begin(), this->header_size_);
    this->f_content_[0] = communication::TLV_TYPE::CONTENT;
    std::advance(this->f_content_, 1);
}

/**
 * Construct an f_message instance std::shared_ptr for a specific file.
 *
 * @param msg_type the message type
 * @param path the file absolute path
 * @param path the file sign
 * @return a new constructed f_message std::shared_ptr instance
 */
std::shared_ptr<communication::f_message> f_message::get_instance(
        MSG_TYPE msg_type,
        fs::path const &path,
        std::string const &sign
) {
    return std::shared_ptr<communication::f_message>(new f_message{msg_type, path, sign});
}


/**
 * Allow to obtain the next file chunk view
 *
 * @return true if the next chunk is available and
 * ready to be used.
 */
bool f_message::next_chunk() {
    if (this->completed_) return false;
    size_t to_read;
    if (this->remaining_ > CHUNK_SIZE - this->header_size_ - 5 - 5) {
        to_read = CHUNK_SIZE - this->header_size_ - 5;
    } else {
        to_read = this->remaining_;
        this->completed_ = true;
    }
    for (int i = 0; i < 4; i++) {
        this->f_content_[i] = (to_read >> (3 - i) * 8) & 0xFF;
    }
    this->ifs_.read(reinterpret_cast<char *>(&*(this->f_content_ + 4)), to_read);
    if (!this->ifs_) {
        throw boost::filesystem::filesystem_error::runtime_error{"Unexpected EOF"};
    }

    if (this->completed_) {
        this->resize(this->header_size_ + 5 + this->remaining_);
        this->add_TLV(communication::TLV_TYPE::END);
        this->ifs_.close();
    }
    this->remaining_ -= to_read;
    return true;
}
