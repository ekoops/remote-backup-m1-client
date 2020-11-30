//
// Created by leonardo on 16/11/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_CONNECTION_H
#define REMOTE_BACKUP_M1_CLIENT_CONNECTION_H


#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/regex.hpp>
#include "message.h"
#include "f_message.h"
#include <mutex>

class connection {
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::ip::tcp::resolver resolver_;
    std::shared_ptr<std::vector<uint8_t>> buffer_;
    boost::asio::io_context::work work_;
    std::vector<std::thread> thread_pool_;

    connection(boost::asio::io_context &io, boost::asio::ssl::context &ctx, size_t thread_pool_size);

public:
    static std::shared_ptr<connection> get_instance(boost::asio::io_context &io,
                                                    boost::asio::ssl::context &ctx,
                                                    size_t thread_pool_size);

    bool verify_certificate(bool preverified, boost::asio::ssl::verify_context &ctx);

    void connect(std::string const &hostname, std::string const &port);

    void join_threads();

    bool write(communication::message const &request_msg);

    std::optional<communication::message> read();


    void post(communication::message const &request_msg,
              std::function<void(communication::message const &)> const &success_fn,
              std::function<void(void)> const &fail_fn
    );

    void post(const std::shared_ptr<communication::f_message> &request_msg,
              std::function<void(communication::message const &)> const &success_fn,
              std::function<void(void)> const &fail_fn);

};

#endif //REMOTE_BACKUP_M1_CLIENT_CONNECTION_H
