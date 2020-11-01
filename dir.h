//
// Created by leonardo on 16/10/20.
//

#ifndef PROVA_WATCHEDDIRECTORY_H
#define PROVA_WATCHEDDIRECTORY_H

#include <boost/filesystem.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <map>
#include <string>
#include <algorithm>
#include <functional>

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

namespace directory {
    class dir {
        boost::filesystem::path path_;
        std::unordered_map<boost::filesystem::path, std::array<uint32_t, 4>> content_;
        bool synced_;
        std::mutex m_;
        dir(boost::filesystem::path path, bool synced = false);
    public:
        dir(dir const &other) = delete;
        dir &operator=(dir const &other) = delete;
        static std::array<uint32_t, 4> hash(boost::filesystem::path const& path);
        static std::shared_ptr<dir> get_instance(boost::filesystem::path const& path, bool synced = false);
        bool insert(boost::filesystem::path const &path);
        bool insert(boost::filesystem::path const &path, std::array<uint32_t, 4> const& hash);
        bool erase(boost::filesystem::path const &path);
        bool update(boost::filesystem::path const &path, std::array<uint32_t, 4> hash);
        bool contains(boost::filesystem::path const &path);
        std::pair<bool, bool> contains_and_match(boost::filesystem::path const &path, std::array<uint32_t, 4> const &hash);
        [[nodiscard]] boost::filesystem::path get_path() const;
        std::unordered_map<boost::filesystem::path, std::array<uint32_t, 4>>* get_content();
        void for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
               std::function<void(std::pair<boost::filesystem::path, std::array<unsigned int, 4>> const &)> const &action);

    };
}

#endif //PROVA_WATCHEDDIRECTORY_H
