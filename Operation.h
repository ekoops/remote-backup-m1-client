//
// Created by leonardo on 15/10/20.
//

#ifndef PROVA_OPERATION_H
#define PROVA_OPERATION_H

#include <string>
#include <boost/filesystem/path.hpp>

enum OPERATION_TYPE {
    CREATE, UPDATE, DELETE
};

class Operation {
    OPERATION_TYPE type;
    boost::filesystem::path path;
public:
    Operation(OPERATION_TYPE type, boost::filesystem::path path);

    OPERATION_TYPE get_type() const;
    boost::filesystem::path get_path() const;
};

namespace std {
    template<>
    struct hash<Operation> {
        size_t operator()(Operation const& operation) const {
            size_t type_hash = std::hash<int>()(operation.get_type());
            size_t path_hash = std::hash<std::string>()(operation.get_path().string()) << 1;
            return type_hash ^ path_hash;
        }
    };
    template<>
    struct equal_to<Operation> {
        bool operator()(Operation const& operation1, Operation const& operation2) const {
            return operation1.get_type() == operation2.get_type() &&
                   operation1.get_path() == operation2.get_path();
        }
    };
}



#endif //PROVA_OPERATION_H
