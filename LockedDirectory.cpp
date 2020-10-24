//
// Created by leonardo on 24/10/20.
//

#include "LockedDirectory.h"

std::shared_ptr<LockedDirectory> LockedDirectory::get_instance(boost::filesystem::path dir_path) {
    return std::shared_ptr<LockedDirectory>(new LockedDirectory {std::move(dir_path)});
}

void LockedDirectory::insert(boost::filesystem::path const &path) {
    std::unique_lock ul{m};
    return Directory::insert(path);
}

bool LockedDirectory::erase(boost::filesystem::path const &path) {
    std::unique_lock ul{m};
    return Directory::erase(path);
}

bool LockedDirectory::update(boost::filesystem::path const& path, Metadata metadata) {
    std::unique_lock ul{m};
    return Directory::update(path, std::move(metadata));
}

std::pair<bool, bool> LockedDirectory::contains(const boost::filesystem::path &path) {
    std::unique_lock ul{m};
    auto it = Directory::get_dir_content().find(path);
    if (it == Directory::get_dir_content().end()) return {false, false};
    else if (it->second != "hash(path)") return {true, true};
    else return {true, false};
}

void LockedDirectory::for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                            std::function<void(std::pair<boost::filesystem::path, Metadata>)> const &action) {
    std::unique_lock ul{m};
    Directory::for_each_if(pred, action);
}