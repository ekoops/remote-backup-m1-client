//
// Created by leonardo on 21/11/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_RESOURCE_H
#define REMOTE_BACKUP_M1_CLIENT_RESOURCE_H

#include <string>
#include <boost/logic/tribool.hpp>
#include <utility>
#include <iostream>

//state: {processing, synced, failed} => synced tribool {indeterminate, true, false}


namespace directory {
    class resource {
        boost::logic::tribool synced_;
        bool exist_on_server_;
        std::string digest_;
    public:
        resource(boost::logic::tribool synced, bool exist_on_server, std::string digest);

        resource synced(boost::logic::tribool const &synced);

        [[nodiscard]] boost::logic::tribool synced() const;

        resource exist_on_server(bool exist_on_server);

        [[nodiscard]] bool exist_on_server() const;

        resource digest(std::string digest);

        [[nodiscard]] std::string digest() const;
    };
    std::ostream& operator<<(std::ostream& os, directory::resource const& rsrc);
}


#endif //REMOTE_BACKUP_M1_CLIENT_RESOURCE_H
