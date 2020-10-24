//
// Created by leonardo on 16/10/20.
//

#include "WatchedDirectory.h"

std::shared_ptr<WatchedDirectory> WatchedDirectory::get_instance(boost::filesystem::path const& path) {
    return std::shared_ptr<WatchedDirectory>(new WatchedDirectory {path});
}

void WatchedDirectory::for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                 std::function<void(std::pair<boost::filesystem::path, std::string>)> const &action) {
    std::unique_lock ul{m};
    for (auto const &element : this->dir_content) {
        if (pred(element.first)) action(element);
    }
}

void WatchedDirectory::insert(boost::filesystem::path const &path) {
    std::unique_lock ul{m};
    this->dir_content[path] = hash(path);
}

void WatchedDirectory::refresh_hash(boost::filesystem::path const &path) {
    std::unique_lock ul{m};
    this->dir_content[path] = hash(path);
}

bool WatchedDirectory::erase(boost::filesystem::path const &path) {
    std::unique_lock ul{m};
    return this->dir_content.erase(path) != 1;
}

// (exists, updated)
std::pair<bool, bool> WatchedDirectory::contains(boost::filesystem::path const &path) {
    std::unique_lock ul{m};
    auto it = this->dir_content.find(path);
    if (it == this->dir_content.end()) return {false, false};
    else if (it->second != hash(path)) return {true, true};
    else return {true, false};
}

std::string WatchedDirectory::hash(boost::filesystem::path const& path) const {
    return "hash";
}

boost::filesystem::path WatchedDirectory::get_dir_path() const {
    return this->dir_path;
}