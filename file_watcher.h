//
// Created by leonardo on 15/10/20.
//

#ifndef PROVA_FILEWATCHER_H
#define PROVA_FILEWATCHER_H

#include <chrono>
#include <string>
#include <boost/filesystem.hpp>
#include <utility>
#include <iostream>
#include <thread>
#include "LockedDirectory.h"
#include <memory>
#include <sstream>
#include "operation_queue.h"


class file_watcher {
    std::chrono::milliseconds delay;
    std::shared_ptr<LockedDirectory> watched_directory;
    std::shared_ptr<operation::operation_queue> poq_;
    bool running_ = true;
public:
    file_watcher(std::shared_ptr<LockedDirectory> watched_directory,
                 std::shared_ptr<operation::operation_queue> poq,
                 std::chrono::milliseconds delay);
    void sync_watched_directory();
    void start();
};


#endif //PROVA_FILEWATCHER_H
