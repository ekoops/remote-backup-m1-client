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

    LockedDirectory(boost::filesystem::path dir_path);
public:
    static std::shared_ptr<LockedDirectory> get_instance(boost::filesystem::path dir_path);
    virtual void insert(boost::filesystem::path const &path) override;
    virtual bool erase(boost::filesystem::path const &path) override;
    virtual bool update(boost::filesystem::path const& path, Metadata metadata) override;
    virtual bool contains(boost::filesystem::path const& path) const override;
    virtual std::pair<bool, bool> contains_and_match(boost::filesystem::path const& path, Metadata const& metadata) const override;
    std::unordered_map<boost::filesystem::path, std::string>& get_dir_content() = delete;
    virtual void for_each_if(std::function<bool(boost::filesystem::path const &)> const &pred,
                                std::function<void(std::pair<boost::filesystem::path, Metadata> const&)> const &action) const override;
};


#endif //REMOTE_BACKUP_M1_CLIENT_LOCKEDDIRECTORY_H
