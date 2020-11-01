//
// Created by leonardo on 31/10/20.
//

#include "operation_queue.h"

using namespace operation;

bool operation_queue::contains(operation::op const& operation) {
    return this->op_set_.find(operation) != this->op_set_.end() ||
           operation == this->pending_op_;
}

std::shared_ptr<operation_queue> operation_queue::get_instance() {
    return std::shared_ptr<operation_queue>(new operation_queue {});
}

void operation_queue::push(operation::op const& operation) {
    std::unique_lock ul{m_};
    if (!this->contains(operation)) {
        if (!this->op_set_.insert(operation).second) {
            throw std::runtime_error{"sciacca"};
        }
        this->op_queue_.push(operation);
        cv_.notify_all();
    }
}
operation::op operation_queue::pop() {
    std::unique_lock ul{m_};
    while (this->op_queue_.empty()) cv_.wait(ul);
    auto operation = this->op_queue_.front();
    if (this->op_set_.erase(operation) != 1) {
        throw std::runtime_error{"sciacca"};
    }
    this->pending_op_ = operation;
    this->op_queue_.pop();
    return this->pending_op_;
}