#ifndef REMOTE_BACKUP_M1_CLIENT_SCHEDULER_H
#define REMOTE_BACKUP_M1_CLIENT_SCHEDULER_H

#include <boost/filesystem.hpp>
#include "connection.h"
#include "../directory/dir.h"
#include "user.h"

/*
 * The scheduler instance will use the connection abstraction
 * to communicate with the server, and a thread pool for constructing
 * in parallel multiple request message. It also manages the user information
 * for reconnection.
 */

class scheduler {
    std::shared_ptr<connection> connection_ptr_;
    std::shared_ptr<directory::dir> dir_ptr_;
    boost::asio::io_context &io_;
    // prevent io_context object's run() call from returning when there is no more work to do
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> ex_work_guard_;
    std::vector<std::thread> thread_pool_;
    // user authentication data
    user user_;

    scheduler(
            boost::asio::io_context &io,
            std::shared_ptr<directory::dir> dir_ptr,
            std::shared_ptr<connection> connection_ptr,
            size_t thread_pool_size
    );

    void handle_create(
            boost::filesystem::path const &relative_path,
            std::string const &sign,
            std::optional<communication::message> const &response
    );

    void handle_update(
            boost::filesystem::path const &relative_path,
            std::string const &sign,
            std::optional<communication::message> const &response
    );

    void handle_erase(
            boost::filesystem::path const &relative_path,
            std::string const &sign,
            std::optional<communication::message> const &response
    );

public:
    static std::shared_ptr<scheduler> get_instance(
            boost::asio::io_context &io,
            std::shared_ptr<directory::dir> dir_ptr,
            std::shared_ptr<connection> connection_ptr,
            size_t thread_pool_size
    );

    void reconnect();

    bool login();

    bool auth(user &usr);

    void sync();

    void create(boost::filesystem::path const &relative_path, std::string const &digest);

    void update(boost::filesystem::path const &relative_path, std::string const &digest);

    void erase(boost::filesystem::path const &relative_path, std::string const &digest);

    void join_threads();
};


#endif //REMOTE_BACKUP_M1_CLIENT_SCHEDULER_H
