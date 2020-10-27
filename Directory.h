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
#include "Metadata.h"

namespace std {
    template<>
    struct hash<boost::filesystem::path> {
        size_t operator()(const boost::filesystem::path &path) const {
            return std::hash<std::string>()(path.string());
        }
    };

    template<>
    struct equal_to<boost::filesystem::path> {
        bool operator()(boost::filesystem::path const &path1, boost::filesystem::path const &path2) const {
            return path1 == path2;
        }
    };
}


class Directory {
    boost::filesystem::path dir_path;
    std::unordered_map<boost::filesystem::path, Metadata> dir_content; // {percorso_del_file, hash_del_file}

    Directory(Directory const& other) = delete;
    Directory& operator=(Directory const& other) = delete;
public:
    Directory(boost::filesystem::path dir_path) : dir_path{std::move(dir_path)} {}
    virtual void insert(boost::filesystem::path const &path);
    virtual bool erase(boost::filesystem::path const &path);
    virtual bool update(boost::filesystem::path const& path, Metadata metadata);
    virtual bool contains(boost::filesystem::path const& path) const;
    virtual std::pair<bool, bool> contains_and_match(boost::filesystem::path const& path, Metadata const& metadata) const;
    boost::filesystem::path get_dir_path() const;
    std::unordered_map<boost::filesystem::path, Metadata>& get_dir_content();
    virtual void for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                     std::function<void(std::pair<boost::filesystem::path, Metadata> const&)> const &action) const;
};

#endif //PROVA_WATCHEDDIRECTORY_H
