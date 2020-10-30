//
// Created by leonardo on 16/10/20.
//

#include "Directory.h"
bool Directory::insert(boost::filesystem::path const& path) {
    return this->dir_content.insert({path, Metadata {path}}).second;
}
bool Directory::erase(boost::filesystem::path const& path) {
    return this->dir_content.erase(path) != 1;
}
bool Directory::update(boost::filesystem::path const& path, Metadata metadata) {
    if (!this->contains(path)) return false;
    if (this->dir_content.insert_or_assign(path, std::move(metadata)).second) {
        throw std::runtime_error {"Insertion shouldn't took place"};
    }
    return true;
}
bool Directory::contains(boost::filesystem::path const& path) {
    return this->dir_content.find(path) != this->dir_content.end();
}
std::pair<bool, bool> Directory::contains_and_match(boost::filesystem::path const& path, Metadata const& metadata) {
    auto it = this->dir_content.find(path);
    if (it == this->dir_content.end()) return {false, false};
    else if (it->second != metadata) return {true, true};
    else return {true, false};
}
boost::filesystem::path Directory::get_dir_path() const {
    return this->dir_path;
}
std::unordered_map<boost::filesystem::path, Metadata>& Directory::get_dir_content() {
    return this->dir_content;
}

void Directory::for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                            std::function<void(std::pair<boost::filesystem::path, Metadata> const&)> const &action) {
    for (auto const &element : this->dir_content) {
        if (pred(element.first)) action(element);
    }
}
