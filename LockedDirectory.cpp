//
// Created by leonardo on 24/10/20.
//

#include "LockedDirectory.h"

LockedDirectory::LockedDirectory(boost::filesystem::path dir_path): Directory {std::move(dir_path)} {}


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
bool LockedDirectory::contains(boost::filesystem::path const& path) const {
    std::unique_lock ul{m};
    return Directory::contains(path);
}
std::pair<bool, bool> LockedDirectory::contains_and_match(boost::filesystem::path const& path, Metadata const& metadata) const {
    std::unique_lock ul{m};
    return Directory::contains_and_match(path, metadata);
}

void LockedDirectory::for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                            std::function<void(std::pair<boost::filesystem::path, Metadata> const&)> const &action) const {
    std::unique_lock ul{m};
    Directory::for_each_if(pred, action);
}