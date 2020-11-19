//
// Created by leonardo on 16/10/20.
//

#include <iostream>
#include "dir.h"
#include "tools.h"

using namespace directory;


dir::dir(boost::filesystem::path path, bool synced) : path_{std::move(path)}, synced_{synced} {}


std::shared_ptr<dir> dir::get_instance(boost::filesystem::path const &path, bool synced) {
    return std::shared_ptr<dir>(new dir{path, synced});
}

bool dir::insert(boost::filesystem::path const &path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    std::cout << "INSERT HASH " << tools::hash(this->path_ / path) << std::endl;
    return this->content_.insert({path, tools::hash(this->path_ / path)}).second;
}

bool dir::insert(boost::filesystem::path const &path, std::string const &digest) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.insert({path, digest}).second;
}

bool dir::erase(boost::filesystem::path const &path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.erase(path) != 1;
}

bool dir::update(boost::filesystem::path const &path, std::string const& digest) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    if (this->content_.find(path) == this->content_.end()) return false;
    if (this->content_.insert_or_assign(path, digest).second) {
        throw std::runtime_error{"Insertion shouldn't took place"};
    }
    return true;
}

bool dir::contains(boost::filesystem::path const &path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.find(path) != this->content_.end();
}

std::pair<bool, bool>
dir::contains_and_match(boost::filesystem::path const &path, std::string const &digest) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    auto it = this->content_.find(path);
    if (it == this->content_.end()) return {false, false};
    else if (it->second != digest) {
        std::cout << it->second << " " << digest << std::endl;
        return {true, false};
    }
    else return {true, true};
}

boost::filesystem::path dir::get_path() const {
    return this->path_;
}

std::unordered_map<boost::filesystem::path, std::string> *dir::get_content() {
    if (this->synced_) return nullptr;
    else return &this->content_;
}

void dir::for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                      std::function<void(
                              std::pair<boost::filesystem::path, std::string const&> const &)> const &action) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    for (auto const &element : this->content_) {
        if (pred(element.first)) action(element);
    }
}
