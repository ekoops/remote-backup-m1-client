//
// Created by leonardo on 21/11/20.
//

#include "resource.h"
#include <boost/logic/tribool_io.hpp>

using namespace directory;

resource::resource(boost::logic::tribool synced, bool exist_on_server, std::string digest) : synced_{synced},
                                                                                             exist_on_server_{
                                                                                                     exist_on_server},
                                                                                             digest_{std::move(
                                                                                                     digest)} {}

resource resource::synced(boost::logic::tribool const &synced) {
    this->synced_ = synced;
    return *this;
}

[[nodiscard]] boost::logic::tribool resource::synced() const {
    return this->synced_;
}

resource resource::exist_on_server(bool exist_on_server) {
    this->exist_on_server_ = exist_on_server;
    return *this;
}

[[nodiscard]] bool resource::exist_on_server() const {
    return this->exist_on_server_;
}

resource resource::digest(std::string digest) {
    this->digest_ = std::move(digest);
    return *this;
}

[[nodiscard]] std::string resource::digest() const {
    return this->digest_;
}

std::ostream& directory::operator<<(std::ostream& os, directory::resource const& rsrc) {
    return os << "{ synced: " << std::boolalpha << rsrc.synced()
            << "; exists_on_server: " << rsrc.exist_on_server()
            << "; digest: " << rsrc.digest() << "}" << std::endl;
}