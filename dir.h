#ifndef REMOTE_BACKUP_M1_CLIENT_DIR_H
#define REMOTE_BACKUP_M1_CLIENT_DIR_H

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/filesystem.hpp>
#include "resource.h"

// Redefinition of hash and equal_to function for boost::filesystem::path
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
        std::unordered_map<boost::filesystem::path, resource> content_;
        bool concurrent_accessed_;
        mutable std::recursive_mutex m_;    // we need to acquire lock also in const methods

    public:
        static std::shared_ptr<dir> get_instance(boost::filesystem::path path, bool concurrent_accessed = false);

        bool insert_or_assign(boost::filesystem::path const &path, resource const& rsrc);

        bool erase(boost::filesystem::path const &path);

        [[nodiscard]] bool contains(boost::filesystem::path const& path) const;

        [[nodiscard]] boost::filesystem::path const& path() const;

        [[nodiscard]] std::optional<resource> rsrc(boost::filesystem::path const &path) const;

        void for_each(std::function<void(std::pair<boost::filesystem::path, directory::resource> const &)> const &fn) const;

    private:
        explicit dir(boost::filesystem::path path, bool concurrent_accessed = false);
    };
}
#endif //REMOTE_BACKUP_M1_CLIENT_DIR_H
