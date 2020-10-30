//
// Created by leonardo on 30/10/20.
//

#ifndef REMOTE_BACKUP_M1_CLIENT_TLV_H
#define REMOTE_BACKUP_M1_CLIENT_TLV_H


#include <cstdint>
#include <vector>
#include <map>
#include <sstream>

enum TLV_TYPE {
    USRN, PSWD, PATH, END
};


struct TLV {
    uint8_t type;
    uint32_t length;
    std::vector<uint8_t> value;
    static std::map<TLV_TYPE, uint8_t> types;
    TLV(uint8_t type,  uint32_t length, char const* data);
    std::vector<uint8_t> serialize() {
        std::vector<uint8_t> ser_tlv;
        ser_tlv.reserve(5 + value.size());
        ser_tlv.push_back(type);
        for (int i=0; i<4; i++) {
            ser_tlv.push_back((length >> (3-i)*8)&0xFF);
        }
        for (auto& raw : value) {
            ser_tlv.push_back(raw);
        }
        return ser_tlv;
    }
};

#endif //REMOTE_BACKUP_M1_CLIENT_TLV_H
