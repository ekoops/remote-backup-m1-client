//
// Created by leonardo on 15/10/20.
//

#include "Operation.h"

Operation::Operation(OPERATION_TYPE type,
                     boost::filesystem::path path)
                     : type{type}, path{std::move(path)} {}

OPERATION_TYPE Operation::get_type() const {return this->type;}
boost::filesystem::path Operation::get_path() const {return this->path;}

bool Operation::operator==(Operation const& other) const {
    return type == other.type && path == other.path;
}

size_t Operation::OperationHashFunction::operator()(const Operation &operation) const {
    size_t typeHash = std::hash<int>()(operation.type);
    size_t pathHash = std::hash<std::string>()(operation.path.string()) << 1;
    return typeHash ^ pathHash;
}