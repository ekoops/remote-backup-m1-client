//
// Created by leonardo on 15/10/20.
//

#include "file_watcher.h"
#include "tools.h"

using namespace boost::filesystem;
using communication::MESSAGE_TYPE;
using communication::TLV_TYPE;
using communication::message_queue;
using directory::dir;

file_watcher::file_watcher(std::shared_ptr<dir> watched_dir,
                           std::shared_ptr<message_queue> poq,
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
    size_t pos = root.size();
    while (running_) {

        std::this_thread::sleep_for(this->wait_time_);

        this->watched_dir_->for_each_if([&root](path const &p) {
            // concatenare i path
            return !exists(root / p);
        }, [this](std::pair<path, std::string> const &pair) {
            std::cout << "DELETE on " << pair.first << "::" << pair.second << " ";
            auto msg = communication::message{MESSAGE_TYPE::DELETE};
            std::string sign = tools::create_sign(pair.first, pair.second);
            msg.add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
            msg.add_TLV(TLV_TYPE::END);
            this->poq_->push(msg);
        });

        // Check if a file was created or modified

        for (auto &file : recursive_directory_iterator(root)) {
            path const& canonical_path = file.path();
            if (is_regular_file(file.path())) {
                path const &relative_path = canonical_path.generic_path().string().substr(pos);
                std::string digest = tools::hash(canonical_path);
                std::string sign = tools::create_sign(relative_path, digest);
                std::pair<bool, bool> pair = this->watched_dir_->contains_and_match(relative_path, digest);
                // if doesn't exists
                if (!pair.first) {
                    std::cout << "CREATE on " << relative_path << "::" << digest << " ";
                    auto msg = communication::message{MESSAGE_TYPE::CREATE};
                    msg.add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
                    msg.add_TLV(TLV_TYPE::FILE, relative_path);
                    msg.add_TLV(TLV_TYPE::END);
                    this->poq_->push(msg);
                    // if hash doesn't match
                } else if (!pair.second) {
                    std::cout << "UPDATE on " << relative_path << "::" << digest << " ";
                    auto msg = communication::message{MESSAGE_TYPE::UPDATE};
                    msg.add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
                    msg.add_TLV(TLV_TYPE::FILE, relative_path);
                    msg.add_TLV(TLV_TYPE::END);
                    this->poq_->push(msg);
                }
            }
        }
    }
}