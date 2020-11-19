//
// Created by leonardo on 31/10/20.
//

#include "message_queue.h"

using namespace communication;

bool message_queue::contains(communication::message const &msg) {
    return this->msg_set_.find(msg) != this->msg_set_.end() || msg == this->pending_msg_;
}

std::shared_ptr<message_queue> message_queue::get_instance() {
    return std::shared_ptr<message_queue>(new message_queue{});
}

void message_queue::push(communication::message const &msg) {
    std::unique_lock ul{m_};
    if (!this->contains(msg)) {
        std::cout << ": INSERTED" << std::endl;
        if (!this->msg_set_.insert(msg).second) {
            throw std::runtime_error{"Failed to enqueue message"};
        }
        this->msg_queue_.push(msg);
        cv_.notify_all();
    } else std::cout << ": REJECTED" << std::endl;
}

communication::message message_queue::pop() {
    std::unique_lock ul{m_};
    while (this->msg_queue_.empty()) cv_.wait(ul);
    auto msg = this->msg_queue_.front();
    if (this->msg_set_.erase(msg) != 1) {
        throw std::runtime_error{"Failed to dequeue message"};
    }
    this->pending_msg_ = msg;
    this->msg_queue_.pop();
    return this->pending_msg_;
}