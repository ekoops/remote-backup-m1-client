#ifndef REMOTE_BACKUP_M1_CLIENT_F_MESSAGE_H
#define REMOTE_BACKUP_M1_CLIENT_F_MESSAGE_H

#include "message.h"


namespace communication {
    /* This class is a specialization of the message class
     * to handle in a more efficient way messages containing
     * file chunks. Specifically it provided on each next_chunk()
     * invocation a new chunk view ready to be sent
     */
    class f_message : public message {
        boost::filesystem::ifstream ifs_;
        std::vector<uint8_t>::iterator f_content_;
        size_t header_size_;
        size_t remaining_;
        bool completed_;
        f_message(MESSAGE_TYPE msg_type,
                  boost::filesystem::path const &path,
                  std::string const &sign);
    public:
        static size_t const CHUNK_SIZE;
        static std::shared_ptr<communication::f_message> get_instance(MESSAGE_TYPE msg_type,
                                                               boost::filesystem::path const &path,
                                                               std::string const &sign);
        bool next_chunk();

    };
}

#endif //REMOTE_BACKUP_M1_CLIENT_F_MESSAGE_H