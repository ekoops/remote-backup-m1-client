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

namespace operation {
    enum OPERATION_TYPE {
        CREATE, UPDATE, DELETE, SYNC, AUTH, NONE
    };

    enum TLV_TYPE {
        USRN, PSWD, PATH, END
    };

    class op {
    public:
        std::shared_ptr<std::vector<uint8_t>> raw_op;
        explicit op(OPERATION_TYPE op_type = OPERATION_TYPE::NONE);
        void add_TLV(TLV_TYPE tlv_type, size_t length=0, char const * buffer = nullptr);
        void write_on_socket(boost::asio::ip::tcp::socket& socket);
        static op read_from_socket(boost::asio::ip::tcp::socket& socket);
        void parse() {
            std::vector<uint8_t>& raw = *this->raw_op;

            auto op_type = static_cast<OPERATION_TYPE>(raw[0]);
            for (int i=1; i<raw.size(); i++) {
                auto tlv_type = static_cast<TLV_TYPE>(raw[i++]);
                size_t length = 0;
                for (int j=0; j<4; j++) {
                    length += raw[i+j] << j*8;
                }

                if ()
            }
        }

        bool operator==(op const& other) const;
        friend std::size_t hash_value(op const &operation);
    };

    std::size_t hash_value(op const &operation) {
        boost::hash<std::vector<uint8_t>> hasher;
        return hasher(*operation.raw_op);
    }
}


#endif //REMOTE_BACKUP_M1_CLIENT_OP_H
