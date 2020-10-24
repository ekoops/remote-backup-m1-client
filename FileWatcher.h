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
#include "PendingOperationsQueue.h"
#include "WatchedDirectory.h"
#include <memory>
#include <sstream>


class FileWatcher {
    std::chrono::milliseconds delay;
    std::shared_ptr<WatchedDirectory> watched_directory;
    bool running_ = true;
public:
    FileWatcher(std::shared_ptr<WatchedDirectory> watched_directory, std::chrono::milliseconds delay);
    void start(PendingOperationsQueue &pendingOperationsQueue);
};


#endif //PROVA_FILEWATCHER_H
