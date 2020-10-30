//
// Created by leonardo on 15/10/20.
//

#include "Operation.h"
Operation::Operation(OPERATION_TYPE type): type {type} {}
Operation::Operation(OPERATION_TYPE type,
                     boost::filesystem::path path)
        : type{type}, path{std::move(path)} {}

OPERATION_TYPE Operation::get_type() const { return this->type; }

boost::filesystem::path Operation::get_path() const { return this->path; }

void Operation::add_TLV(TLV_TYPE type, size_t length, char const *buffer) {
//    this->tlv_chain.emplace_back(TLV{
//            static_cast<uint8_t>(type),
//            static_cast<uint32_t>(length),
//            buffer
//    });
}