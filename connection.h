class scheduler;
#ifndef REMOTE_BACKUP_M1_CLIENT_CONNECTION_H
#define REMOTE_BACKUP_M1_CLIENT_CONNECTION_H

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/regex.hpp>
#include <mutex>
#include "f_message.h"
#include "message.h"
#include "user.h"
#include "scheduler.h"

class connection {
    // needed for isolated completion handler execution
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::resolver::results_type endpoints_;
    // prevent io_context object's run() call from returning when there is no more work to do
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> ex_work_guard_;
    std::vector<std::thread> thread_pool_;
    boost::asio::steady_timer keepalive_timer_;
    std::weak_ptr<scheduler> scheduler_ptr_;
    user user_;

public:
    static std::shared_ptr<connection> get_instance(
            boost::asio::io_context &io,
            boost::asio::ssl::context &ctx,
            size_t thread_pool_size
    );

    void resolve(std::string const &hostname, std::string const &service);

    void connect();

    void schedule_keepalive();
    void cancel_keepalive();

        std::optional<communication::message> sync_post(communication::message const &request_msg);

    void post(communication::message const &request_msg,
              std::function<void(std::optional<communication::message> const &)> const &fn);

    void post(std::shared_ptr<communication::f_message> const &request_msg,
              std::function<void(std::optional<communication::message> const &)> const &fn);

    void close();

    void set_scheduler(const std::shared_ptr<scheduler> &scheduler_ptr);
    void set_user(user usr);

        private:
    connection(
            boost::asio::io_context &io,
            boost::asio::ssl::context &ctx,
            size_t thread_pool_size
    );

    bool write(communication::message const &request_msg);

    std::optional<communication::message> read();

};

#endif //REMOTE_BACKUP_M1_CLIENT_CONNECTION_H
