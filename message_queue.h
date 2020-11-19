//
// Created by leonardo on 31/10/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_MESSAGE_QUEUE_H
#define REMOTE_BACKUP_M1_CLIENT_MESSAGE_QUEUE_H

#include <queue>
#include <unordered_set>
#include <condition_variable>
#include "message.h"
#include <boost/functional/hash.hpp>
#include <boost/unordered_set.hpp>

namespace communication {

    class message_queue {
        std::queue<message> msg_queue_;
//        boost::unordered_set<message, boost::hash<message>> msg_set_;
        std::unordered_set<message, boost::hash<message>> msg_set_;
        message pending_msg_;
        std::condition_variable cv_;
        std::mutex m_;

        bool contains(message const &msg);

        message_queue() = default;

    public:
        message_queue(message_queue const &other) = delete;

        message_queue& operator=(message_queue const &other) = delete;

        static std::shared_ptr<message_queue> get_instance();

        void push(message const &msg);

        message pop();
    };
}


#endif //REMOTE_BACKUP_M1_CLIENT_MESSAGE_QUEUE_H
