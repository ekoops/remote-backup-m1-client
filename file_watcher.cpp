//
// Created by leonardo on 15/10/20.
//

#include "file_watcher.h"

#include <utility>
#include "tools.h"
#include "resource.h"

namespace fs = boost::filesystem;

file_watcher::file_watcher(std::shared_ptr<directory::dir> dir_ptr,
                           std::shared_ptr<scheduler> scheduler_ptr,
                           std::chrono::milliseconds wait_time)
        : dir_ptr_{std::move(dir_ptr)}, scheduler_ptr_{std::move(scheduler_ptr)}, wait_time_{wait_time} {
    size_t watched_dir_length = this->dir_ptr_->path().size();
    for (auto &de : fs::recursive_directory_iterator(dir_ptr_->path())) {
        fs::path const &absolute_path = de.path();
        if (fs::is_regular_file(absolute_path)) {
            boost::filesystem::path relative_path{absolute_path.generic_path().string().substr(watched_dir_length)};
            this->dir_ptr_->insert_or_assign(relative_path, directory::resource{
                    boost::indeterminate,
                    false,
                    tools::hash(absolute_path, relative_path)
            });
        }
    }
    this->scheduler_ptr_->schedule_sync();
}

void file_watcher::start() {
    fs::path const &root = this->dir_ptr_->path();
    size_t watched_dir_length = root.size();
    while (this->running_) {
        std::this_thread::sleep_for(this->wait_time_);

        this->dir_ptr_->for_each([&root, this](std::pair<fs::path, directory::resource> const &pair) {
            if (fs::exists(root / pair.first)) return;
            fs::path const &relative_path = pair.first;
            directory::resource const &rsrc = pair.second;

            if (!boost::indeterminate(rsrc.synced())) {
                if (rsrc.exist_on_server()) {
                    this->scheduler_ptr_->schedule_delete(relative_path, rsrc.digest());
                } else {
                    this->dir_ptr_->erase(pair.first);
                }
            }
        });

        for (auto &file : fs::recursive_directory_iterator(root)) {
            fs::path const &absolute_path = file.path();
            if (fs::is_regular_file(absolute_path)) {
                fs::path relative_path{absolute_path.generic_path().string().substr(watched_dir_length)};
                std::string digest = tools::hash(absolute_path, relative_path);

                // if doesn't exists
                if (!this->dir_ptr_->contains(relative_path)) {
                    this->scheduler_ptr_->schedule_create(relative_path, digest);
                } else {
                    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
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