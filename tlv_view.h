#ifndef REMOTE_BACKUP_M1_CLIENT_TLV_VIEW_H
#define REMOTE_BACKUP_M1_CLIENT_TLV_VIEW_H

#include <vector>
#include "message.h"

namespace communication {
/** This class provide an easy way to iterate over each
 * TLV segment of a message. It is necessary to call
 * next_tlv at least one time to obtain a valid TLV
 * segment view; otherwise, all the accessor method
 * will throw an exception. On each valid next_tlv()
 * it is possible to access to T, L and V through
 * the methods tlv_type(), length() and begin()/cbegin()/end()/cend()
 */
    class tlv_view {
        TLV_TYPE tlv_type_;
        size_t length_;
        std::shared_ptr<std::vector<uint8_t>> raw_msg_ptr_;
        std::vector<uint8_t>::iterator begin_, end_;
        std::vector<uint8_t>::iterator data_begin_, data_end_;
        bool valid_;
        bool finished_;
    public:
        tlv_view(message const &msg);

        bool next_tlv();

        [[nodiscard]] bool valid() const;

        [[nodiscard]] bool finished() const;

        [[nodiscard]] communication::TLV_TYPE tlv_type() const;

        [[nodiscard]] size_t length() const;

        std::vector<uint8_t>::iterator begin();

        std::vector<uint8_t>::iterator end();

        [[nodiscard]] std::vector<uint8_t>::const_iterator cbegin() const;

        std::vector<uint8_t>::const_iterator cend() const;
    };
}


#endif //REMOTE_BACKUP_M1_CLIENT_TLV_VIEW_H
