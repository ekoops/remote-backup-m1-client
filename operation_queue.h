//
// Created by leonardo on 31/10/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_OPERATION_QUEUE_H
#define REMOTE_BACKUP_M1_CLIENT_OPERATION_QUEUE_H

#include <queue>
#include <unordered_set>
#include <condition_variable>
#include "op.h"
#include <boost/functional/hash.hpp>


namespace operation {

    class operation_queue {
        std::queue<op> op_queue_;
        std::unordered_set<op, boost::hash<op>> op_set_;
        op pending_op_;
        std::condition_variable cv_;
        std::mutex m_;

        bool contains(op const &operation);

        operation_queue() = default;

    public:
        operation_queue(operation_queue const &other) = delete;

        operation_queue &operator=(operation_queue const &other) = delete;

        static std::shared_ptr<operation_queue> get_instance();

        void push(op const &operation);

        op pop();
    };
}


#endif //REMOTE_BACKUP_M1_CLIENT_OPERATION_QUEUE_H
