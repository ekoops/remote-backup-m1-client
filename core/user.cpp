#include "user.h"

/**
 * Construct a user instance.
 *
 * @param username the user username
 * @param password the user password
 * @return a new constructed user instance
 */
user::user(std::string username, std::string password)
: username_ {std::move(username)}
, password_ {std::move(password)} {}

/**
* Getter for username field.
*
* @return the username field reference
*/
[[nodiscard]] std::string const& user::username() const {
    return this->username_;
}

/**
* Getter for password field.
*
* @return the password field reference
*/
[[nodiscard]] std::string const& user::password() const {
    return this->password_;
}

/**
* Allow to verify if the user is authenticated.
*
* @return true if the user is authenticated, false otherwise
*/
[[nodiscard]] bool user::authenticated() const {
    return this->authenticated_;
}

/**
* Allow to set the user authentication state
*
* @param authenticated the new user authentication state
* @return void
*/
void user::authenticated(bool authenticated) {
    this->authenticated_ = authenticated;
}
