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
    std::unordered_map<boost::filesystem::path, std::string> dir_content; // {percorso_del_file, hash_del_file}
    //TODO implementarla tramite std::unordered_map
    std::mutex m;

    WatchedDirectory(boost::filesystem::path dir_path) : dir_path{std::move(dir_path)} {}

public:

    static std::shared_ptr<WatchedDirectory> get_instance(boost::filesystem::path const &path);
    void for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                     std::function<void(std::pair<boost::filesystem::path, std::string>)> const &action);
    void insert(boost::filesystem::path const &path);
    void refresh_hash(boost::filesystem::path const &path);
    bool erase(boost::filesystem::path const &path);
    std::pair<bool, bool> contains(boost::filesystem::path const &path);
    std::string hash(boost::filesystem::path const &path) const;
    boost::filesystem::path get_dir_path() const;
};

#endif //PROVA_WATCHEDDIRECTORY_H
