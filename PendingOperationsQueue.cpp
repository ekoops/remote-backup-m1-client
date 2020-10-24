//
// Created by leonardo on 15/10/20.
//

#include "PendingOperationsQueue.h"

bool PendingOperationsQueue::contains(Operation const& operation) {
    return this->operations_set.find(operation) != operations_set.end();
}

void PendingOperationsQueue::insert(Operation const& operation) {
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

Operation PendingOperationsQueue::next_op() {
    std::unique_lock ul {m};
    while (this->operations_queue.empty()) cv.wait(ul);
    auto operation = this->operations_queue.front();
    if (this->operations_set.erase(operation) != 1) {
        throw std::runtime_error {"sciacca"};
    }
    this->operations_queue.pop();
    return operation;
}