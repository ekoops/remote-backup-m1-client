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
#include "dir2.h"
#include "scheduler.h"


class file_watcher {
    std::chrono::milliseconds wait_time_;
    std::shared_ptr<directory::dir2> dir_ptr_;
    std::shared_ptr<scheduler> scheduler_ptr_;
    bool running_ = true;


public:
    file_watcher(std::shared_ptr<directory::dir2> dir_ptr,
                 std::shared_ptr<scheduler> scheduler_ptr,
                 std::chrono::milliseconds wait_time);

    void start();
};

#endif //PROVA_FILEWATCHER_H
