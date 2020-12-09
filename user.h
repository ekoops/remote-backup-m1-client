//
// Created by leonardo on 08/12/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_USER_H
#define REMOTE_BACKUP_M1_CLIENT_USER_H
#include <string>

class user {
    std::string username_;
    std::string password_;
    bool authenticated_ = false;
public:
    user() = default;
    user(std::string username, std::string password)
    : username_ {std::move(username)}
    , password_ {std::move(password)} {}

    [[nodiscard]] std::string const& username() const {
        return this->username_;
    }
    [[nodiscard]] std::string const& password() const {
        return this->password_;
    }
    void authenticated(bool authenticated) {
        this->authenticated_ = authenticated;
    }
    [[nodiscard]] bool authenticated() const {
        return this->authenticated_;
    }
};


#endif //REMOTE_BACKUP_M1_CLIENT_USER_H
