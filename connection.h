//
// Created by leonardo on 16/11/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_CONNECTION_H
#define REMOTE_BACKUP_M1_CLIENT_CONNECTION_H


#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/regex.hpp>
#include "message.h"

class connection {
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver resolver_;
public:
    connection(boost::asio::io_context &io);

    void connect(std::string const &hostname, std::string const &portno);

    bool authenticate();

    void write(communication::message const &msg);
    communication::message read();
};

#endif //REMOTE_BACKUP_M1_CLIENT_CONNECTION_H
