//
// Created by leonardo on 16/10/20.
//

#include "dir.h"

#include <utility>
#include <iostream>

using namespace directory;

using boost::uuids::detail::md5;

dir::dir(boost::filesystem::path path, bool synced) : path_{std::move(path)}, synced_{synced} {}

std::array<uint32_t, 4> dir::hash(boost::filesystem::path const &path) {
    std::ifstream ifs{path};
    std::string s;
    md5 hash;
    md5::digest_type digest;
    while (std::getline(ifs, s)) {
        hash.process_bytes(s.data(), s.size());
    }
    hash.get_digest(digest);
    return std::array<unsigned int, 4>{
            digest[0],
            digest[1],
            digest[2],
            digest[3]
    };

}

std::shared_ptr<dir> dir::get_instance(boost::filesystem::path const &path, bool synced) {
    return std::shared_ptr<dir>(new dir{path, synced});
}

bool dir::insert(boost::filesystem::path const &path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    if (!boost::filesystem::exists(path)) return false;

    return this->content_.insert({path, dir::hash(path)}).second;
}

bool dir::insert(boost::filesystem::path const &path, std::array<uint32_t, 4> const &hash) {
    this->content_.insert({path, hash}).second;
}

bool dir::erase(boost::filesystem::path const &path) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    return this->content_.erase(path) != 1;
}

bool dir::update(boost::filesystem::path const &path, std::array<uint32_t, 4> hash) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    if (!this->contains(path)) return false;
    if (this->content_.insert_or_assign(path, hash).second) {
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
dir::contains_and_match(boost::filesystem::path const &path, std::array<uint32_t, 4> const &hash) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    auto it = this->content_.find(path);
    if (it == this->content_.end()) return {false, false};
    else if (it->second != hash) return {true, true};
    else return {true, false};
}

boost::filesystem::path dir::get_path() const {
    return this->path_;
}

std::unordered_map<boost::filesystem::path, std::array<uint32_t, 4>> *dir::get_content() {
    if (this->synced_) return nullptr;
    else return &this->content_;
}

void dir::for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                      std::function<void(
                              std::pair<boost::filesystem::path, std::array<uint32_t, 4>> const &)> const &action) {
    std::unique_lock ul{this->m_, std::defer_lock};
    if (synced_) ul.lock();
    for (auto const &element : this->content_) {
        if (pred(element.first)) action(element);
    }
}
