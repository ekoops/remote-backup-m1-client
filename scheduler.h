#ifndef REMOTE_BACKUP_M1_CLIENT_SCHEDULER_H
#define REMOTE_BACKUP_M1_CLIENT_SCHEDULER_H

#include <boost/filesystem.hpp>
#include "connection.h"
#include "dir.h"

/*
 * This class allows to schedule operation based
 * on server communication
 */
class scheduler {
    std::shared_ptr<connection> connection_ptr_;
    std::shared_ptr<directory::dir> dir_ptr_;
    boost::asio::io_context &io_;
    // prevent io_context object's run() call from returning when there is no more work to do
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> ex_work_guard_;
    std::vector<std::thread> thread_pool_;

    scheduler(
            boost::asio::io_context &io,
            std::shared_ptr<directory::dir> dir_ptr,
            std::shared_ptr<connection> connection_ptr,
            size_t thread_pool_size
    );

    void handle_sync(std::optional<communication::message> const &response);

    void handle_create(boost::filesystem::path const &relative_path,
                       std::string const &sign,
                       std::optional<communication::message> const &response);

    void handle_update(boost::filesystem::path const &relative_path,
                       std::string const &sign,
                       std::optional<communication::message> const &response);

    void handle_erase(boost::filesystem::path const &relative_path,
                      std::string const &sign,
                      std::optional<communication::message> const &response);

public:
    static std::shared_ptr<scheduler> get_instance(
            boost::asio::io_context &io,
            std::shared_ptr<directory::dir> dir_ptr,
            std::shared_ptr<connection> connection_ptr,
            size_t thread_pool_size
    );

    bool auth(std::string const &username, std::string const &password);

    void sync();

    void create(boost::filesystem::path const &relative_path, std::string const &digest);

    void update(boost::filesystem::path const &relative_path, std::string const &digest);

    void erase(boost::filesystem::path const &relative_path, std::string const &digest);

    void close();
};


#endif //REMOTE_BACKUP_M1_CLIENT_SCHEDULER_H
