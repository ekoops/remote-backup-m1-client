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
#include <boost/regex.hpp>
#include "message.h"

class connection {
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver resolver_;
    size_t header_;
    std::shared_ptr<std::vector<uint8_t>> buffer_;

    void parse_payload(const boost::system::error_code &e,
                       std::function<void(communication::message const&)> const &callback);

    void handle_payload_write(const boost::system::error_code &e,
                              communication::message const &msg,
                              std::function<void(void)> const &callback);

    void handle_header_write(const boost::system::error_code &e,
                             communication::message const &msg,
                             std::function<void(void)> const &callback);

    void handle_payload_read(const boost::system::error_code &e,
                             std::function<void(communication::message const&)> const &callback);

    void handle_header_read(const boost::system::error_code &e,
                            std::function<void(communication::message const&)> const &callback);

    connection(boost::asio::io_context &io);
public:
    static std::shared_ptr<connection> get_instance(boost::asio::io_context &io);

    void connect(std::string const &hostname, std::string const &portno);

    bool authenticate();

    void write(communication::message const &msg);

    communication::message read();

    void async_write(communication::message const &msg, std::function<void(void)> const &callback);

    void async_read(std::function<void(communication::message const&)> const &callback);
};

#endif //REMOTE_BACKUP_M1_CLIENT_CONNECTION_H