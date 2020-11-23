//
// Created by leonardo on 15/10/20.
//

#include "file_watcher.h"

#include <utility>
#include "tools.h"
#include "resource.h"

using namespace boost::filesystem;
using communication::MESSAGE_TYPE;
using communication::TLV_TYPE;

file_watcher::file_watcher(std::shared_ptr<directory::dir2> dir_ptr,
                           std::shared_ptr<scheduler> scheduler_ptr,
                           std::chrono::milliseconds wait_time)
        : dir_ptr_{std::move(dir_ptr)}, scheduler_ptr_{std::move(scheduler_ptr)}, wait_time_{wait_time} {
    size_t pos = this->dir_ptr_->path().size();
    for (auto &de : recursive_directory_iterator(dir_ptr_->path())) {
        boost::filesystem::path const &absolute_path = de.path();
        if (is_regular_file(absolute_path)) {
            boost::filesystem::path relative_path{absolute_path.generic_path().string().substr(pos)};
            dir_ptr_->insert_or_assign(relative_path, directory::resource{
                boost::indeterminate,
                false,
                tools::hash(absolute_path, relative_path)
            });
        }
    }

}

//void file_watcher::start() {
//    using namespace boost::filesystem;
//    path const &root = this->watched_dir_->get_path().generic_path();
//    size_t pos = root.size();
//    while (running_) {
//
//        std::this_thread::sleep_for(this->wait_time_);
//
//        this->watched_dir_->for_each_if([&root](path const &p) {
//            // concatenare i path
//            return !exists(root / p);
//        }, [this](std::pair<path, std::string> const &pair) {
//            std::cout << "DELETE on " << pair.first << "::" << pair.second << " ";
//            auto msg = communication::message{MESSAGE_TYPE::DELETE};
//            std::string sign = tools::create_sign(pair.first, pair.second);
//            msg.add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
//            msg.add_TLV(TLV_TYPE::END);
//            this->poq_->push(msg);
//        });
//
//        // Check if a file was created or modified
//
//        for (auto &file : recursive_directory_iterator(root)) {
//            path const &canonical_path = file.path();
//            if (is_regular_file(file.path())) {
//                path const &relative_path = canonical_path.generic_path().string().substr(pos);
//                std::string digest = tools::hash(canonical_path);
//                std::string sign = tools::create_sign(relative_path, digest);
//                std::pair<bool, bool> pair = this->watched_dir_->contains_and_match(relative_path, digest);
//                // if doesn't exists
//                if (!pair.first) {
//                    std::cout << "CREATE on " << relative_path << "::" << digest << " ";
//                    auto msg = communication::message{MESSAGE_TYPE::CREATE};
//                    msg.add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
//                    msg.add_TLV(TLV_TYPE::FILE, relative_path);
//                    msg.add_TLV(TLV_TYPE::END);
//                    // async_write(handle_create)
//                    // synced, failed, processing
//                    // file 1: synced false; last_op: CREATE, PATH: PATH, HASH: HASH
//                    this->poq_->push(msg);
//                    // if hash doesn't match
//                } else if (!pair.second) {
//                    std::cout << "UPDATE on " << relative_path << "::" << digest << " ";
//                    auto msg = communication::message{MESSAGE_TYPE::UPDATE};
//                    msg.add_TLV(TLV_TYPE::ITEM, sign.size(), sign.c_str());
//                    msg.add_TLV(TLV_TYPE::FILE, relative_path);
//                    msg.add_TLV(TLV_TYPE::END);
//                    this->poq_->push(msg);
//                }
//            }
//        }
//    }
//}



void file_watcher::start() {
    using namespace boost::filesystem;
    path const &root = this->dir_ptr_->path().generic_path();
    size_t pos = root.size();
    while (running_) {

        std::this_thread::sleep_for(this->wait_time_);

        this->dir_ptr_->for_each([&root, this](std::pair<path, directory::resource> const &pair) {
            if (exists(root / pair.first)) return;
            boost::filesystem::path const &relative_path = pair.first;
            directory::resource const &rsrc = pair.second;
            if (!boost::logic::indeterminate(rsrc.synced())) {
                if (rsrc.exist_on_server()) {
                    this->scheduler_ptr_->schedule_delete(relative_path, rsrc.digest());
                } else {
                    this->dir_ptr_->erase(pair.first);
                }
            }
        });

        for (auto &file : recursive_directory_iterator(root)) {
            path const &canonical_path = file.path();
            if (is_regular_file(canonical_path)) {
                boost::filesystem::path relative_path{canonical_path.generic_path().string().substr(pos)};
                std::string digest = tools::hash(canonical_path, relative_path);

                // if doesn't exists
                if (!this->dir_ptr_->contains(relative_path)) {
                    this->scheduler_ptr_->schedule_create(relative_path, digest);
                } else {
                    auto rsrc = this->dir_ptr_->rsrc(relative_path).value();
                    if (rsrc.synced() == true) {
                        if (rsrc.digest() != digest) this->scheduler_ptr_->schedule_update(relative_path, digest);
                    } else if (rsrc.synced() == false) {
                        if (rsrc.exist_on_server()) this->scheduler_ptr_->schedule_update(relative_path, digest);
                        else this->scheduler_ptr_->schedule_create(relative_path, digest);
                    }
                }
            }
        }
    }
}