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
    std::queue<Operation> operations_queue;
    std::unordered_set<Operation, Operation::OperationHashFunction> operations_set;
    std::condition_variable cv;
    std::mutex m;

    bool contains(Operation const& operation);
public:
    void insert(Operation const& operation);
    Operation next_op();
};


#endif //PROVA_PENDINGOPERATIONSQUEUE_H
