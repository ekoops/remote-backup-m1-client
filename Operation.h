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

    bool operator==(Operation const& other) const;
    struct OperationHashFunction {
        size_t operator()(const Operation& operation) const;
    };
};



#endif //PROVA_OPERATION_H
