//
// Created by leonardo on 15/10/20.
//

#include "FileWatcher.h"

using namespace boost::filesystem;

FileWatcher::FileWatcher(std::shared_ptr<LockedDirectory> watched_directory,
                         std::shared_ptr<PendingOperationsQueue> poq,
                         std::chrono::milliseconds delay)
        : watched_directory{std::move(watched_directory)}, poq{std::move(poq)}, delay{delay} {
    for (auto &de : recursive_directory_iterator(watched_directory->get_dir_path())) {
        watched_directory->insert(de.path());
    }
}

void FileWatcher::sync_watched_directory() {
    boost::filesystem::path path_from_server;
    //TODO obtain server_directory from server

    Directory server_directory{path_from_server};
    for (auto &de : server_directory.get_dir_content()) {
        std::pair<bool, bool> pair = this->watched_directory->contains_and_match(de.first, de.second);
        // if doesn't exist
        if (!pair.first) {
            auto op = Operation::get_instance(OPERATION_TYPE::DELETE);
            op->add_TLV(TLV_TYPE::PATH, sizeof(de.first.generic_path()), de.first.generic_path().c_str());
            this->poq->push(op);
            // if updated
        } else if (pair.second) {
            auto op = Operation::get_instance(OPERATION_TYPE::UPDATE);
            op->add_TLV(TLV_TYPE::PATH, sizeof(de.first.generic_path()), de.first.generic_path().c_str());
            this->poq->push(op);
        }
    }
    this->watched_directory->for_each_if([&server_directory](boost::filesystem::path const &path) {
        return !server_directory.contains(path);
    }, [this](std::pair<boost::filesystem::path, Metadata> const &pair) {
        auto op = Operation::get_instance(OPERATION_TYPE::CREATE);
        op->add_TLV(TLV_TYPE::PATH, sizeof(pair.first.generic_path()), pair.first.generic_path().c_str());
        this->poq->push(op);
    });
}

// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
void FileWatcher::start() {
    using namespace boost::filesystem;
    while (running_) {
        std::this_thread::sleep_for(delay);

        this->watched_directory->for_each_if([](path const &p) {
            return !exists(p);
        }, [this](std::pair<boost::filesystem::path, Metadata> const &pair) {
            auto op = Operation::get_instance(OPERATION_TYPE::DELETE);
            op->add_TLV(TLV_TYPE::PATH, sizeof(pair.first.generic_path()), pair.first.generic_path().c_str());
            this->poq->push(op);
        });

        // Check if a file was created or modified
        for (auto &file : recursive_directory_iterator(this->watched_directory->get_dir_path())) {
            path const &p = file.path();
            Metadata metadata = Metadata{p};
            std::pair<bool, bool> pair = this->watched_directory->contains_and_match(p, metadata);
            // if not exists
            if (!pair.first) {
                auto op = Operation::get_instance(OPERATION_TYPE::CREATE);
                op->add_TLV(TLV_TYPE::PATH, sizeof(p.generic_path()), p.generic_path().c_str());
                this->poq->push(op);
            // if updated
            } else if (pair.second) {
                auto op = Operation::get_instance(OPERATION_TYPE::UPDATE);
                op->add_TLV(TLV_TYPE::PATH, sizeof(p.generic_path()), p.generic_path().c_str());
                this->poq->push(op);
            }
        }
    }
}