//
// Created by leonardo on 30/10/20.
//

#include "TLV.h"

std::map<TLV_TYPE, uint8_t> TLV::types = {
        {USRN, 1},
        {PSWD, 2},
};

TLV::TLV(uint8_t type, uint32_t length, char const* data) : type {type}, length {length} {
    for (int i=0; i<length; i++) this->value.push_back(data[i]);
}