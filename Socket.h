//
// Created by leonardo on 27/10/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_SOCKET_H
#define REMOTE_BACKUP_M1_CLIENT_SOCKET_H


#include <sys/socket.h>
#include <stdexcept>
#include <unistd.h>

class Socket {
    int sockfd;
    Socket(Socket const&) = delete;
    Socket &operator=(Socket const&) = delete;
public:
    Socket();
    Socket(Socket&& other);
    Socket &operator=(Socket&& other);
    ~Socket();

    ssize_t read(char *buffer, size_t len, int options) const;
    ssize_t write(char const* buffer, size_t len, int options) const;
    void connect(struct sockaddr_in *addr, unsigned int len);
};


#endif //REMOTE_BACKUP_M1_CLIENT_SOCKET_H
