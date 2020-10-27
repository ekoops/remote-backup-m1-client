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
#include "LockedDirectory.h"
#include <memory>
#include <sstream>


class FileWatcher {
    std::chrono::milliseconds delay;
    std::shared_ptr<LockedDirectory> watched_directory;
    std::shared_ptr<PendingOperationsQueue> poq;
    bool running_ = true;
public:
    FileWatcher(std::shared_ptr<LockedDirectory> watched_directory,
                std::shared_ptr<PendingOperationsQueue> poq,
                std::chrono::milliseconds delay);
    void sync_watched_directory();
    void start();
};


#endif //PROVA_FILEWATCHER_H
