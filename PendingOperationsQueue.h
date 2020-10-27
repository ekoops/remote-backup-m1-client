//
// Created by leonardo on 15/10/20.
//

#ifndef PROVA_PENDINGOPERATIONSQUEUE_H
#define PROVA_PENDINGOPERATIONSQUEUE_H


#include <queue>
#include <unordered_set>
#include <condition_variable>
#include "Operation.h"

class PendingOperationsQueue {
    std::queue<std::shared_ptr<Operation>> operations_queue;
    std::unordered_set<std::shared_ptr<Operation>> operations_set;
    std::shared_ptr<Operation> pending_op;
    std::condition_variable cv;
    std::condition_variable leonardo;
    std::mutex m;

    bool contains(std::shared_ptr<Operation> op_ptr);
    PendingOperationsQueue() = default;
public:
    static std::shared_ptr<PendingOperationsQueue> get_instance();
    void push(std::shared_ptr<Operation> op_ptr);
    std::shared_ptr<Operation> pop();
};


#endif //PROVA_PENDINGOPERATIONSQUEUE_H
