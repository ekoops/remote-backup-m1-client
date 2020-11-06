//
// Created by leonardo on 30/10/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_OP_H
#define REMOTE_BACKUP_M1_CLIENT_OP_H
#include <iostream>
#include <vector>
#include <cstdint>
#include <mutex>
#include <boost/functional/hash.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem/fstream.hpp>

namespace operation {
    enum OPERATION_TYPE {
        CREATE, UPDATE, DELETE, SYNC, AUTH, NONE
    };

    enum TLV_TYPE {
        USRN, PSWD, ITEM, END, OK, ERROR, PATH, FILE
    };

    class op {
        std::shared_ptr<std::vector<uint8_t>> raw_op_;
    public:
        explicit op(OPERATION_TYPE op_type = OPERATION_TYPE::NONE);
        void add_TLV(TLV_TYPE tlv_type, size_t length=0, char const * buffer = nullptr);
        void add_TLV(TLV_TYPE tlv_type, boost::filesystem::path const& path);
            void write_on_socket(boost::asio::ip::tcp::socket& socket);
        static op read_from_socket(boost::asio::ip::tcp::socket& socket);
        [[nodiscard]] std::shared_ptr<std::vector<uint8_t>> get_raw_op() const;
        [[nodiscard]] OPERATION_TYPE get_op_type() const;

        bool operator==(op const& other) const;
        friend std::size_t hash_value(op const &operation);
    };
}


#endif //REMOTE_BACKUP_M1_CLIENT_OP_H
