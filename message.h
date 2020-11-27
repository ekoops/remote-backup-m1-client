//
// Created by leonardo on 30/10/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_MESSAGE_H
#define REMOTE_BACKUP_M1_CLIENT_MESSAGE_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <mutex>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/serialization/vector.hpp>

namespace communication {
    enum MESSAGE_TYPE {
        NONE, CREATE, UPDATE, DELETE, SYNC, AUTH
    };

    enum TLV_TYPE {
        USRN, PSWD, ITEM, END, OK, ERROR, PATH, CONTENT
    };

    class message {
        std::shared_ptr<std::vector<uint8_t>> raw_msg_;
    public:
        explicit message(MESSAGE_TYPE msg_type = MESSAGE_TYPE::NONE);

        explicit message(size_t length);

        explicit message(MESSAGE_TYPE msg_type, size_t length);

        explicit message(std::shared_ptr<std::vector<uint8_t>> raw_msg_);

        void add_TLV(TLV_TYPE tlv_type, size_t length = 0, char const *buffer = nullptr);

        void add_TLV(TLV_TYPE tlv_type, boost::filesystem::path const &path);

        [[nodiscard]] std::shared_ptr<std::vector<uint8_t>> get_raw_msg_ptr() const;

        [[nodiscard]] MESSAGE_TYPE get_msg_type() const;

        boost::asio::mutable_buffer buffer() const;

        size_t size() const;

        void resize(size_t length);

        bool operator==(message const &other) const;

        template<class Archive>
        void serialize(Archive &ar, unsigned int version) {
            ar & (*this->raw_msg_);
        }
    };

    std::ostream &operator<<(std::ostream &os, communication::message const &msg);

//    std::size_t hash_value(communication::message const &msg);
}


#endif //REMOTE_BACKUP_M1_CLIENT_MESSAGE_H
