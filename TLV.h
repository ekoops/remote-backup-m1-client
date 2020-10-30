//
// Created by leonardo on 30/10/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_TLV_H
#define REMOTE_BACKUP_M1_CLIENT_TLV_H


#include <cstdint>
#include <vector>

struct TLV {
    uint8_t type;
    uint32_t length;
    std::vector<uint8_t> value;

    TLV(uint8_t type, uint32_t length, char* data): type {type}, length {length} {
        for (int i=0; i<length; i++) {
            value.push_back(data[i]);
        }
    }
};

#endif //REMOTE_BACKUP_M1_CLIENT_TLV_H
