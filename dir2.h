//
// Created by leonardo on 21/11/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_DIR2_H
#define REMOTE_BACKUP_M1_CLIENT_DIR2_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/filesystem.hpp>
#include "resource.h"

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
    class dir2 {
        boost::filesystem::path path_;
        std::unordered_map<boost::filesystem::path, resource> content_;
        bool synced_;
        std::recursive_mutex m_;

        dir2(boost::filesystem::path path, bool synced = false);

    public:
        static std::shared_ptr<dir2> get_instance(boost::filesystem::path const &path, bool synced = false);

        bool insert_or_assign(boost::filesystem::path const &path, resource rsrc);

        bool erase(boost::filesystem::path const &path);

        bool contains(boost::filesystem::path const& path);

        [[nodiscard]] boost::filesystem::path path() const;

        [[nodiscard]] std::optional<resource> rsrc(boost::filesystem::path const &path);

        void for_each(std::function<void(std::pair<boost::filesystem::path, directory::resource> const &)> const &fn);
    };
}
#endif //REMOTE_BACKUP_M1_CLIENT_DIR2_H
