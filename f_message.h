//
// Created by leonardo on 25/11/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_F_MESSAGE_H
#define REMOTE_BACKUP_M1_CLIENT_F_MESSAGE_H

#include "message.h"


namespace communication {
    class f_message : public message {
        static size_t const CHUNK_SIZE = 4096;
        boost::filesystem::ifstream ifs_;
        std::vector<uint8_t>::iterator f_content_;
        size_t header_size_;
        size_t f_length_;
        size_t remaining_;
        f_message(MESSAGE_TYPE msg_type,
                  boost::filesystem::path const &path,
                  std::string const &sign);
    public:
        static std::shared_ptr<communication::f_message> get_instance(MESSAGE_TYPE msg_type,
                                                               boost::filesystem::path const &path,
                                                               std::string const &sign);
        bool next_chunk();

    };
}

#endif //REMOTE_BACKUP_M1_CLIENT_F_MESSAGE_H