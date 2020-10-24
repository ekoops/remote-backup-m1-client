//
// Created by leonardo on 24/10/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_LOCKEDDIRECTORY_H
#define REMOTE_BACKUP_M1_CLIENT_LOCKEDDIRECTORY_H
#include <mutex>

#include "Directory.h"

class LockedDirectory : public Directory {
    std::mutex m;
    LockedDirectory(LockedDirectory const& other) = delete;
    LockedDirectory& operator=(LockedDirectory const& other) = delete;
    LockedDirectory(boost::filesystem::path dir_path): Directory {std::move(dir_path)} {}
public:
    static std::shared_ptr<LockedDirectory> get_instance(boost::filesystem::path dir_path);
    void insert(boost::filesystem::path const &path);
    bool erase(boost::filesystem::path const &path);
    bool update(boost::filesystem::path const& path, Metadata metadata);
    std::pair<bool, bool> contains(boost::filesystem::path const &path);
    std::unordered_map<boost::filesystem::path, std::string>& get_dir_content() = delete;
    void for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                                std::function<void(std::pair<boost::filesystem::path, Metadata>)> const &action);
};


#endif //REMOTE_BACKUP_M1_CLIENT_LOCKEDDIRECTORY_H
