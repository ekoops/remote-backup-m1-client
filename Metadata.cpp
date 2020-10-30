//
// Created by leonardo on 24/10/20.
//

#include "Metadata.h"

#include <utility>

using namespace boost::filesystem;

Metadata::Metadata(boost::filesystem::path path, TYPE type, uintmax_t size, size_t hash)
    : path {std::move(path)}
    , type {type}
    , size {size}
    , hash {hash}{

}

Metadata::Metadata(boost::filesystem::path const& path)
    : path {path}
    , type {is_directory(path) ? TYPE::FOLDER_T : is_regular_file(path) ? TYPE::FILE_T : TYPE::UNKNOWN_T}
    , size {file_size(path)}
    , hash {Metadata::create_hash(path)}
    {}

size_t Metadata::create_hash(const boost::filesystem::path &path) {
    //TODO
    return 1;
}

bool Metadata::operator!=(Metadata const& other) const {
    return this->hash != other.hash;
}
