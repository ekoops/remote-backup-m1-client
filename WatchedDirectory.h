//
// Created by leonardo on 16/10/20.
//

#ifndef PROVA_WATCHEDDIRECTORY_H
#define PROVA_WATCHEDDIRECTORY_H

#include <boost/filesystem.hpp>
#include <map>
#include <string>
#include <algorithm>
#include <functional>


class WatchedDirectory {
    boost::filesystem::path dir_path;
    std::map<boost::filesystem::path, std::string> dir_content; // {percorso_del_file, hash_del_file}
    //TODO implementarla tramite std::unordered_map
    std::mutex m;

    WatchedDirectory(boost::filesystem::path dir_path) : dir_path{std::move(dir_path)} {}
public:

    static std::shared_ptr<WatchedDirectory> get_instance(boost::filesystem::path const& path) {
        return std::shared_ptr<WatchedDirectory>(new WatchedDirectory {path});
    }

    void for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                     std::function<void(std::pair<boost::filesystem::path, std::string>)> const &action) {
        std::unique_lock ul{m};
        for (auto const &element : this->dir_content) {
            if (pred(element.first)) action(element);
        }
    }

    void insert(boost::filesystem::path const &path) {
        std::unique_lock ul{m};
        this->dir_content[path] = hash(path);
    }

    void refresh_hash(boost::filesystem::path const &path) {
        std::unique_lock ul{m};
        this->dir_content[path] = hash(path);
    }

    bool erase(boost::filesystem::path const &path) {
        std::unique_lock ul{m};
        return this->dir_content.erase(path) != 1;
    }

    // (exists, updated)
    std::pair<bool, bool> contains(boost::filesystem::path const &path) const {
        std::unique_lock ul{m};
        auto it = this->dir_content.find(path);
        if (it == this->dir_content.end()) return {false, false};
        else if (it->second != hash(path)) return {true, true};
        else return {true, false};
    }

    std::string hash(boost::filesystem::path const& path) const {
        return "hash";
    }

    boost::filesystem::path get_dir_path() const {
        return this->dir_path;
    };
}

#endif //PROVA_WATCHEDDIRECTORY_H
