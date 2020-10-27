//
// Created by leonardo on 15/10/20.
//

#include "PendingOperationsQueue.h"

bool PendingOperationsQueue::contains(std::shared_ptr<Operation> op_ptr) {
    return this->operations_set.find(op_ptr) != operations_set.end() ||
        op_ptr == this->pending_op;
}

std::shared_ptr<PendingOperationsQueue> PendingOperationsQueue::get_instance() {
    return std::shared_ptr<PendingOperationsQueue>(new PendingOperationsQueue{});
}

void PendingOperationsQueue::push(std::shared_ptr<Operation> op_ptr) {
    std::unique_lock ul{m};
    //TODO devo controllare la presenza dell'operazione anche nel Worker thread
    while (this->operations_queue.size() == 1024) {
        cv.notify_all();
        leonardo.wait(ul);
    }
    if (!this->contains(op_ptr)) {
        // .second ritorna true se l'inserimento è riuscito
        if (!this->operations_set.insert(op_ptr).second) {
            throw std::runtime_error{"sciacca"};
        }
        this->operations_queue.push(op_ptr);
        cv.notify_all();
    }
}

std::shared_ptr<Operation> PendingOperationsQueue::pop() {
    std::unique_lock ul{m};
    while (this->operations_queue.empty()) {
        leonardo.notify_all();
        cv.wait(ul);
    }
    auto op_ptr = this->operations_queue.front();
    if (this->operations_set.erase(op_ptr) != 1) {
        throw std::runtime_error{"sciacca"};
    }
    this->pending_op = op_ptr;
    this->operations_queue.pop();
    leonardo.notify_all();
    return this->pending_op;
}