//
// Created by leonardo on 16/10/20.
//

#include "Directory.h"
void Directory::insert(boost::filesystem::path const& path) {
    this->dir_content[path] = hash(path);
}
bool Directory::erase(boost::filesystem::path const& path) {
    return this->dir_content.erase(path) != 1;
}
bool Directory::update(boost::filesystem::path const& path, Metadata metadata) {
    if (!this->contains(path)) return false;
    this->dir_content[path] = std::move(metadata);
}
bool Directory::contains(boost::filesystem::path const& path) const {
    return this->dir_content.find(path) != this->dir_content.end();
}
boost::filesystem::path Directory::get_dir_path() const {
    return this->dir_path;
}
std::unordered_map<boost::filesystem::path, Metadata>& Directory::get_dir_content() {
    return this->dir_content;
}

void Directory::for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                            std::function<void(std::pair<boost::filesystem::path, Metadata>)> const &action) {
    for (auto const &element : this->dir_content) {
        if (pred(element.first)) action(element);
    }
}
