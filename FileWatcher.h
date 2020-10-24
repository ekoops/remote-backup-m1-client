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
    bool running_ = true;
public:
    FileWatcher(std::shared_ptr<LockedDirectory> watched_directory, std::chrono::milliseconds delay);
    void start(PendingOperationsQueue &pendingOperationsQueue);
    void sync_watched_directory() {
        boost::filesystem::path path_from_server;
        Directory server_directory {path_from_server};
        for (auto& de : server_directory.get_dir_content()) {
            std::pair<bool, bool> results = this->watched_directory->contains(de.first);
            if ()
        }
        //get server directory
//        Directory server_directory;
//        watched_directory
    }
};


#endif //PROVA_FILEWATCHER_H
