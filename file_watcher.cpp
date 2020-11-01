//
// Created by leonardo on 15/10/20.
//

#include "file_watcher.h"

using namespace boost::filesystem;
using operation::OPERATION_TYPE;
using operation::TLV_TYPE;
using operation::operation_queue;
using operation::op;
using directory::dir;

file_watcher::file_watcher(std::shared_ptr<dir> watched_dir,
                           std::shared_ptr<operation_queue> poq,
                           std::chrono::milliseconds wait_time)
        : watched_dir_{std::move(watched_dir)}, poq_{std::move(poq)}, wait_time_{wait_time} {
    for (auto &de : recursive_directory_iterator(watched_dir_->get_path())) {
        watched_dir_->insert(de.path());
    }
}

void file_watcher::sync_watched_dir() {
    boost::filesystem::path path_from_server;
    //TODO obtain server_directory from server

    std::shared_ptr<directory::dir> server_dir = directory::dir::get_instance(path_from_server);
    for (auto &de : *(server_dir->get_content())) {
        std::pair<bool, bool> pair = this->watched_dir_->contains_and_match(de.first, de.second);
        // if doesn't exist
        if (!pair.first) {
            auto op = operation::op{OPERATION_TYPE::DELETE};
            op.add_TLV(TLV_TYPE::PATH, sizeof(de.first.generic_path()), de.first.generic_path().c_str());
            this->poq_->push(op);
            // if updated
        } else if (pair.second) {
            auto op = operation::op{OPERATION_TYPE::UPDATE};
            op.add_TLV(TLV_TYPE::PATH, sizeof(de.first.generic_path()), de.first.generic_path().c_str());
            this->poq_->push(op);
        }
    }
    this->watched_dir_->for_each_if([&server_dir](boost::filesystem::path const &path) {
        return !server_dir->contains(path);
    }, [this](std::pair<boost::filesystem::path, Metadata> const &pair) {
        auto op = operation::op{OPERATION_TYPE::CREATE};
        op.add_TLV(TLV_TYPE::PATH, sizeof(pair.first.generic_path()), pair.first.generic_path().c_str());
        this->poq_->push(op);
    });
}

// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
void file_watcher::start() {
    using namespace boost::filesystem;
    while (running_) {
        std::this_thread::sleep_for(this->wait_time_);

        this->watched_dir_->for_each_if([](path const &p) {
            return !exists(p);
        }, [this](std::pair<boost::filesystem::path, Metadata> const &pair) {
            auto op = operation::op{OPERATION_TYPE::DELETE};
            op.add_TLV(TLV_TYPE::PATH, sizeof(pair.first.generic_path()), pair.first.generic_path().c_str());
            this->poq_->push(op);
        });

        // Check if a file was created or modified
        for (auto &file : recursive_directory_iterator(this->watched_dir_->get_path())) {
            path const &p = file.path();
            Metadata metadata = Metadata{p};
            std::pair<bool, bool> pair = this->watched_dir_->contains_and_match(p, metadata);
            // if not exists
            if (!pair.first) {
                auto op = operation::op{OPERATION_TYPE::CREATE};
                op.add_TLV(TLV_TYPE::PATH, sizeof(p.generic_path()), p.generic_path().c_str());
                this->poq_->push(op);
                // if updated
            } else if (pair.second) {
                auto op = operation::op{OPERATION_TYPE::UPDATE};
                op.add_TLV(TLV_TYPE::PATH, sizeof(p.generic_path()), p.generic_path().c_str());
                this->poq_->push(op);
            }
        }
    }
}