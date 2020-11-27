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
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket_;
    boost::asio::ip::tcp::resolver resolver_;
    size_t header_;
    std::shared_ptr<std::vector<uint8_t>> buffer_;
    boost::asio::io_context::work work_;
    std::vector<std::thread> thread_group_;
    std::mutex m_;
    std::condition_variable cv_;
    bool can_write = true;
    size_t read_header_;

    void handle_payload_write(const boost::system::error_code &e,
                              communication::message const &msg,
                              std::function<void(void)> const &success_callback,
                              std::function<void(void)> const &fail_callback);

    void handle_header_write(const boost::system::error_code &e,
                             communication::message const &msg,
                             std::function<void(void)> const &success_callback,
                             std::function<void(void)> const &fail_callback);

    void parse_payload(const boost::system::error_code &e,
                       std::function<void(communication::message const &)> const &success_callback,
                       std::function<void(void)> const &fail_callback);

    void handle_payload_read(const boost::system::error_code &e,
                             std::function<void(communication::message const &)> const &success_callback,
                             std::function<void(void)> const &fail_callback);

    void handle_header_read(const boost::system::error_code &e,
                            std::function<void(communication::message const &)> const &success_callback,
                            std::function<void(void)> const &fail_callback);

    connection(boost::asio::io_context &io, boost::asio::ssl::context& ctx);

public:
    static std::shared_ptr<connection> get_instance(boost::asio::io_context &io,
                                                    boost::asio::ssl::context& ctx);

    void connect(std::string const &hostname, std::string const &portno);

    bool write(communication::message const &msg);

    std::optional<communication::message> read();

    void async_write(communication::message const &msg,
                     std::function<void(void)> const &success_callback,
                     std::function<void(void)> const &fail_callback);

    void async_read(
            std::function<void(communication::message const &)> const &success_callback,
            std::function<void(void)> const &fail_callback);


    void post(communication::message const &request_msg,
                          std::function<void(communication::message const&)> const& fn);
    void post(communication::message const &request_msg,
              std::function<void(communication::message const&)> const& success_fn,
              std::function<void(void)> const& fail_fn
              );
    void post(const std::shared_ptr<communication::f_message> &request_msg,
                          std::function<void(communication::message const &)> const &success_fn,
                          std::function<void(void)> const &fail_fn);

    void execute_callback(boost::system::error_code const &e, std::function<void(communication::message const&)> const &fn);
        void read_response(boost::system::error_code const &e, std::function<void(communication::message const&)> const &fn);

    void async_write2(communication::message const &msg, std::function<void(communication::message const&)> const &fn);
};

#endif //REMOTE_BACKUP_M1_CLIENT_CONNECTION_H
