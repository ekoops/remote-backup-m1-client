#include "resource.h"
#include <boost/logic/tribool_io.hpp>

using namespace directory;

/**
 * Construct a resource instance setting up all its field
 *
 * @param synced value specifying if client is synced with server on this resource
 * @param exist_on_server value specifying if the client knows that this resource exist on server
 * @param digest the resource digest
 * @return a new constructed connection instance
 */
resource::resource(
        boost::logic::tribool synced,
        bool exist_on_server,
        std::string digest)
        : synced_{synced}
        , exist_on_server_{exist_on_server}
        , digest_{std::move(digest)} {}

/**
* Setter for synced field.
*
* @param synced the new synced field value
* @return the resource on which it has been applied
*/
resource &resource::synced(boost::logic::tribool const &synced) {
    this->synced_ = synced;
    return *this;
}

/**
* Getter for synced field.
*
* @return the synced field value
*/
[[nodiscard]] boost::logic::tribool resource::synced() const {
    return this->synced_;
}

/**
* Setter for exist_on_server field.
*
* @param exist_on_server the new exist_on_server field value
* @return the resource on which it has been applied
*/
resource &resource::exist_on_server(bool exist_on_server) {
    this->exist_on_server_ = exist_on_server;
    return *this;
}

/**
* Getter for exist_on_server field.
*
* @return the exist_on_server field value
*/
[[nodiscard]] bool resource::exist_on_server() const {
    return this->exist_on_server_;
}

/**
* Setter for digest field.
*
* @param digest the new digest field value
* @return the resource on which it has been applied
*/
resource &resource::digest(std::string digest) {
    this->digest_ = std::move(digest);
    return *this;
}

/**
* Getter for digest field.
*
* @return the digest field value
*/
[[nodiscard]] std::string resource::digest() const {
    return this->digest_;
}

std::ostream &directory::operator<<(std::ostream &os, directory::resource const &rsrc) {
    return os << "{ synced: " << std::boolalpha << rsrc.synced()
              << "; exists_on_server: " << rsrc.exist_on_server()
              << "; digest: " << rsrc.digest() << "}" << std::endl;
}