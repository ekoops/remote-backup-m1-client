//
// Created by leonardo on 15/10/20.
//

#include "Operation.h"

Operation::Operation(OPERATION_TYPE type,
                     boost::filesystem::path path)
                     : type{type}, path{std::move(path)} {}

OPERATION_TYPE Operation::get_type() const {return this->type;}
boost::filesystem::path Operation::get_path() const {return this->path;}
