//
// Created by leonardo on 15/10/20.
//

#include "file_watcher.h"
#include "tools.h"

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
    size_t pos = this->watched_dir_->get_path().size();
    for (auto &de : recursive_directory_iterator(watched_dir_->get_path())) {
        path const& p = de.path();
        if (is_regular_file(p)) {
            watched_dir_->insert(p.generic_path().string().substr(pos));
        }
    }
}

// Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
void file_watcher::start() {
    using namespace boost::filesystem;
    path const& root = this->watched_dir_->get_path().generic_path();
    while (running_) {

        std::this_thread::sleep_for(this->wait_time_);

        this->watched_dir_->for_each_if([&root](path const &p) {
            // concatenare i path
            return !exists(root / p);
        }, [this](std::pair<path, std::string> const &pair) {
            auto op = operation::op{OPERATION_TYPE::DELETE};
            std::string sign = tools::create_sign(pair.first, pair.second);
            op.add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
            op.add_TLV(TLV_TYPE::END);
            this->poq_->push(op);
        });

        // Check if a file was created or modified

        for (auto &file : recursive_directory_iterator(root)) {
            path const &path = file.path();
            std::string digest = tools::hash(path);
            std::string sign = tools::create_sign(path, digest);
            std::pair<bool, bool> pair = this->watched_dir_->contains_and_match(path, digest);
            // if doesn't exists
            if (!pair.first) {
                auto op = operation::op{OPERATION_TYPE::CREATE};
                op.add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
                op.add_TLV(TLV_TYPE::FILE, path);
                op.add_TLV(TLV_TYPE::END);
                this->poq_->push(op);
                // if hash doesn't match
            } else if (!pair.second) {
                auto op = operation::op{OPERATION_TYPE::UPDATE};
                op.add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
                op.add_TLV(TLV_TYPE::FILE, path);
                op.add_TLV(TLV_TYPE::END);
                this->poq_->push(op);
            }
        }
    }
}