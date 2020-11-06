//
// Created by leonardo on 15/10/20.
//

#ifndef PROVA_FILEWATCHER_H
#define PROVA_FILEWATCHER_H

#include <chrono>
#include <string>
#include <boost/filesystem.hpp>
#include <iostream>
#include <thread>
#include <memory>
#include <sstream>
#include "operation_queue.h"
#include "dir.h"


class file_watcher {
    std::chrono::milliseconds wait_time_;
    std::shared_ptr<directory::dir> watched_dir_;
    std::shared_ptr<operation::operation_queue> poq_;
    bool running_ = true;
public:
    file_watcher(std::shared_ptr<directory::dir> watched_dir,
                 std::shared_ptr<operation::operation_queue> poq,
                 std::chrono::milliseconds wait_time);
    void start();
};


#endif //PROVA_FILEWATCHER_H
