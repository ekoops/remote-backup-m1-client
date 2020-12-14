#ifndef PROVA_FILEWATCHER_H
#define PROVA_FILEWATCHER_H

#include <chrono>
#include <string>
#include <boost/filesystem.hpp>
#include <iostream>
#include <thread>
#include <memory>
#include <sstream>
#include <mutex>
#include "../directory/dir.h"
#include "scheduler.h"

/**
 * This class allow to create a file_watcher given a specific
 * directory. If a specific resource is not synced, this class
 * uses the associated scheduler to schedule the appropriate
 * operation.
 */
class file_watcher {
    std::chrono::milliseconds wait_time_;
    std::shared_ptr<directory::dir> dir_ptr_;
    std::shared_ptr<scheduler> scheduler_ptr_;
    bool running_ = true;
public:
    file_watcher(std::shared_ptr<directory::dir> dir_ptr,
                 std::shared_ptr<scheduler> scheduler_ptr,
                 std::chrono::milliseconds wait_time);
    void start();
};

#endif //PROVA_FILEWATCHER_H
