//
// Created by leonardo on 21/11/20.
//

#include "dir.h"

using namespace directory;

dir::dir(boost::filesystem::path path, bool synced) : path_{std::move(path)}, synced_{synced} {}


std::shared_ptr<dir> dir::get_instance(boost::filesystem::path const &path, bool synced) {
    return std::shared_ptr<dir>(new dir{path, synced});
}

bool dir::insert_or_assign(boost::filesystem::path const &path, resource rsrc) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.insert_or_assign(path, rsrc).second;
}

bool dir::erase(boost::filesystem::path const& path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.erase(path) == 1;
}

bool dir::contains(boost::filesystem::path const& path) const {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.find(path) != this->content_.end();
}

boost::filesystem::path dir::path() const {
    return this->path_;
}

std::optional<resource> dir::rsrc(boost::filesystem::path const& path) const {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    auto it = this->content_.find(path);
    return it != this->content_.end() ? std::optional<directory::resource>{it->second} : std::nullopt;

}

void dir::for_each(std::function<void(std::pair<boost::filesystem::path, directory::resource> const&)> const& fn) const {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    std::for_each(this->content_.cbegin(), this->content_.cend(), fn);
}