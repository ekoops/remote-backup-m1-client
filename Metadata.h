//
// Created by leonardo on 24/10/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_METADATA_H
#define REMOTE_BACKUP_M1_CLIENT_METADATA_H
#include <string>
#include <boost/filesystem.hpp>

enum TYPE {
    FOLDER_T, FILE_T, UNKNOWN_T
};

class Metadata {
    boost::filesystem::path path;
    TYPE type;
    uintmax_t size;
    size_t hash;
public:
    Metadata(boost::filesystem::path const& path);
    static size_t create_hash(boost::filesystem::path const& path);
};


#endif //REMOTE_BACKUP_M1_CLIENT_METADATA_H
