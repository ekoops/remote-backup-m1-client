//
// Created by leonardo on 21/11/20.
//

#include "dir2.h"

using namespace directory;

dir2::dir2(boost::filesystem::path path, bool synced) : path_{std::move(path)}, synced_{synced} {}


std::shared_ptr<dir2> dir2::get_instance(boost::filesystem::path const &path, bool synced) {
    return std::shared_ptr<dir2>(new dir2{path, synced});
}

bool dir2::insert_or_assign(boost::filesystem::path const &path, resource rsrc) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.insert_or_assign(path, rsrc).second;
}

bool dir2::erase(boost::filesystem::path const& path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.erase(path) == 1;
}

bool dir2::contains(boost::filesystem::path const& path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.find(path) != this->content_.end();
}

boost::filesystem::path dir2::path() const {
    return this->path_;
}

std::optional<resource> dir2::rsrc(boost::filesystem::path const& path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    auto it = this->content_.find(path);
    return it != this->content_.end() ? std::optional<directory::resource>{it->second} : std::nullopt;

}


void dir2::for_each(std::function<void(std::pair<boost::filesystem::path, directory::resource> const&)> const& fn) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    std::for_each(this->content_.cbegin(), this->content_.cend(), fn);
}