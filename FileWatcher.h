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


class FileWatcher {
    std::chrono::seconds delay;
    std::shared_ptr<WatchedDirectory> watched_directory;
    bool running_ = true;
public:
    FileWatcher(std::shared_ptr<WatchedDirectory> watched_directory, std::chrono::seconds delay)
            : delay{delay}, watched_directory{watched_directory} {
        for (auto &de : boost::filesystem::recursive_directory_iterator(watched_directory->get_dir_path())) {
            watched_directory->insert(de.path());
        }
        //richiedere al server modello_filesystem e generare operazioni iniziali

    }

    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(PendingOperationsQueue &pendingOperationsQueue) {
        using namespace boost::filesystem;
        while (running_) {
            // Wait for "delay" milliseconds
            std::this_thread::sleep_for(delay);


            // Check if a file was removed
            //standard associative-container erase idiom
            this->watched_directory->for_each_if(
                    [](path const &p) { return exists(p); },
                    [&pendingOperationsQueue](std::pair<boost::filesystem::path, std::string> const &pair) {
                        Operation op{OPERATION_TYPE::DELETE, pair.first};
                        pendingOperationsQueue.insert(std::move(op));
                    });

            // Check if a file was created or modified
            for (auto &file : recursive_directory_iterator(this->watched_directory->get_dir_path())) {
                path p = file.path();
                std::pair<bool, bool> pair = this->watched_directory->contains(p);
                // if not exists
                if (!pair.first) {
                    Operation op{OPERATION_TYPE::CREATE, p};
                    pendingOperationsQueue.insert(std::move(op));
                    // if updated
                } else if (pair.second) {
                    Operation op{OPERATION_TYPE::UPDATE, p};
                    pendingOperationsQueue.insert(std::move(op));
                }
            }
        }
    }
};


#endif //PROVA_FILEWATCHER_H
