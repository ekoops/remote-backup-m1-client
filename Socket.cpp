//
// Created by leonardo on 27/10/20.
//

#include "Socket.h"

Socket::Socket() {
    this->sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd < 0) throw std::runtime_error("Cannot create socket");
}
Socket::Socket(Socket&& other): sockfd {other.sockfd} {
other.sockfd = 0;
}
Socket &Socket::operator=(Socket&& other) {
    if (this->sockfd != 0) close(this->sockfd);
    this->sockfd = other.sockfd;
    other.sockfd = 0;
    return *this;
}
Socket::~Socket() {
    if (this->sockfd != 0) close(this->sockfd);
}

ssize_t Socket::read(char *buffer, size_t len, int options) const {
    ssize_t res = recv(this->sockfd, buffer, len, options);
    if (res < 0) throw std::runtime_error("Cannot receive from socket");
    return res;
}
ssize_t Socket::write(char const* buffer, size_t len, int options) const {
    ssize_t res = send(this->sockfd, buffer, len, options);
    if (res < 0) throw std::runtime_error("Cannot write to socket");
    return res;
}
void Socket::connect(struct sockaddr_in *addr, unsigned int len) {
    if (::connect(this->sockfd, reinterpret_cast<struct sockaddr *>(addr), len) != 0) {
        throw std::runtime_error("Cannot connect to remote socket");
    }
}