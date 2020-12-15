#ifndef REMOTE_BACKUP_M1_CLIENT_CONNECTION_H
#define REMOTE_BACKUP_M1_CLIENT_CONNECTION_H

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/bind/bind.hpp>
#include <boost/signals2.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/regex.hpp>
#include <mutex>
#include "../communication/f_message.h"
#include "../communication/message.h"
#include "user.h"

/*
 * This class provides an abstraction for an SSL socket.
 * It allow to communicate with server using the sync_post
 * and the async_post methods. It manages asynchronous
 * calls using the internal strand and the internal thread. It also handles keepalive
 * and reconnection features.
 */
class connection {
    // needed for isolated completion handler execution
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::resolver::results_type endpoints_;
    // prevent io_context object's run() calls from returning when there is no more work to do
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> ex_work_guard_;
    std::vector<std::thread> thread_pool_;
    std::thread thread_;
    boost::asio::steady_timer keepalive_timer_;
    boost::signals2::signal<void()> handle_reconnection_;
public:

    static std::shared_ptr<connection> get_instance(
            boost::asio::io_context &io,
            boost::asio::ssl::context &ctx
    );

    void resolve(std::string const &hostname, std::string const &service);

    void connect();

    void schedule_keepalive();

    void cancel_keepalive();

    std::pair<
            boost::logic::tribool,
            std::optional<communication::message>
    > sync_post(communication::message const &request_msg);

    void async_post(
            communication::message const &request_msg,
            std::function<void(std::optional<communication::message> const &)> const &fn
    );

    void async_post(
            std::shared_ptr<communication::f_message> const &request_msg,
            std::function<void(std::optional<communication::message> const &)> const &fn
    );

    void set_reconnection_handler(std::function<void(void)> const &fn);

    void join_thread();

private:
    connection(
            boost::asio::io_context &io,
            boost::asio::ssl::context &ctx
    );

    boost::logic::tribool write(communication::message const &request_msg);

    std::pair<boost::logic::tribool, std::optional<communication::message>> read();

};

#endif //REMOTE_BACKUP_M1_CLIENT_CONNECTION_H
