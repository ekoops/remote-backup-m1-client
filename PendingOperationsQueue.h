//
// Created by leonardo on 15/10/20.
//

#ifndef PROVA_PENDINGOPERATIONSQUEUE_H
#define PROVA_PENDINGOPERATIONSQUEUE_H


#include <queue>
#include <set>
#include <condition_variable>
#include "Operation.h"

class PendingOperationsQueue {
    std::queue<Operation> operations_queue;
    std::set<Operation> operations_set;
    // unordered_set
    std::condition_variable cv;
    std::mutex m;

    bool contains(Operation const& operation) {
        return this->operations_set.find(operation) != operations_set.end();
    }
public:
    PendingOperationsQueue() {}

    ~PendingOperationsQueue() {}


    void insert(Operation operation) {
        std::unique_lock ul {m};
        //TODO devo controllare la presenza dell'operazione anche nel Worker thread
        if (!this->contains(operation)) {
            // .second ritorna true se l'inserimento è riuscito
            if (!this->operations_set.insert(operation).second) {
                throw std::runtime_error {"sciacca"};
            }
            this->operations_queue.push(operation);
            cv.notify_all();
        }
    }

    Operation next_op() {
        std::unique_lock ul {m};
        while (this->operations_queue.empty()) cv.wait(ul);
        auto operation = this->operations_queue.front();
        if (this->operations_set.erase(operation) != 1) {
            throw std::runtime_error {"sciacca"};
        }
        this->operations_queue.pop();
        return operation;
    }
};


#endif //PROVA_PENDINGOPERATIONSQUEUE_H
