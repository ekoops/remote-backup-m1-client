//
// Created by leonardo on 22/11/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_SCHEDULER_H
#define REMOTE_BACKUP_M1_CLIENT_SCHEDULER_H

#include <boost/filesystem.hpp>
#include "connection.h"
#include "dir.h"


class scheduler {
    std::shared_ptr<connection> connection_ptr_;
    std::shared_ptr<directory::dir> dir_ptr_;


    scheduler(std::shared_ptr<directory::dir> dir_ptr, std::shared_ptr<connection> connection_ptr);

    void handle_sync(communication::message const &response_msg);

    void handle_create(boost::filesystem::path const &relative_path, std::string const &sign,
                       communication::message const &response_msg);

    void handle_update(boost::filesystem::path const &relative_path, std::string const &sign,
                       communication::message const &response_msg);

    void handle_delete(boost::filesystem::path const &relative_path, std::string const &sign,
                       communication::message const &response_msg);
public:
    static std::shared_ptr<scheduler> get_instance(std::shared_ptr<directory::dir> dir_ptr, std::shared_ptr<connection> connection_ptr );

    bool try_auth(std::string const &username, std::string const &password);

    void schedule_sync();

    void schedule_create(boost::filesystem::path const &relative_path, std::string const &digest);

    void schedule_update(boost::filesystem::path const &relative_path, std::string const &digest);

    void schedule_delete(boost::filesystem::path const &relative_path, std::string const &digest);

    static void
    fail_callback(const std::shared_ptr<directory::dir> &dir_ptr_, boost::filesystem::path const &relative_path);

};


#endif //REMOTE_BACKUP_M1_CLIENT_SCHEDULER_H
