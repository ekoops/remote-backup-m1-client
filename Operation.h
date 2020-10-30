//
// Created by leonardo on 15/10/20.
//

#ifndef PROVA_OPERATION_H
#define PROVA_OPERATION_H

#include <string>
#include <boost/filesystem/path.hpp>
#include "TLV.h"

enum OPERATION_TYPE {
    CREATE, UPDATE, DELETE, SYNC, AUTH
};

class Operation {
    OPERATION_TYPE type;
    std::vector<TLV> tlv_chain;
    boost::filesystem::path path;
    Operation(OPERATION_TYPE type, boost::filesystem::path path);
public:
    static std::shared_ptr<Operation> get_instance(OPERATION_TYPE type, boost::filesystem::path path) {
        return std::shared_ptr<Operation>(new Operation {type, std::move(path)});
    }
    bool operator==(Operation const& other) const {
        return this->get_type() == other.get_type() &&
                      this->get_path() == other.get_path();
    }
    OPERATION_TYPE get_type() const;
    boost::filesystem::path get_path() const;
};

namespace std {
    template<>
    struct hash<std::shared_ptr<Operation>> {
        size_t operator()(std::shared_ptr<Operation> const& operation) const {
            size_t type_hash = std::hash<int>()(operation->get_type());
            size_t path_hash = std::hash<std::string>()(operation->get_path().string()) << 1;
            return type_hash ^ path_hash;
        }
    };
    template<>
    struct equal_to<std::shared_ptr<Operation>> {
        bool operator()(std::shared_ptr<Operation> const& operation1, std::shared_ptr<Operation> const& operation2) const {
            return *operation1 == *operation2;
        }
    };
}



#endif //PROVA_OPERATION_H
