//
// Created by leonardo on 15/10/20.
//

#include "FileWatcher.h"

using namespace boost::filesystem;

FileWatcher::FileWatcher(std::shared_ptr<LockedDirectory> watched_directory, std::chrono::milliseconds delay)
        : delay{delay}, watched_directory{watched_directory} {
    for (auto &de : recursive_directory_iterator(watched_directory->get_dir_path())) {
        watched_directory->insert(de.path());
    }
    //richiedere al server modello_filesystem e generare operazioni iniziali

}

// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
void FileWatcher::start(PendingOperationsQueue &pendingOperationsQueue) {
    using namespace boost::filesystem;
    while (running_) {
        // Wait for "delay" milliseconds
        std::this_thread::sleep_for(delay);

        this->watched_directory->for_each_if(
                [](path const &p) { return !exists(p); },
                [&pendingOperationsQueue](std::pair<boost::filesystem::path, Metadata> const &pair) {
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