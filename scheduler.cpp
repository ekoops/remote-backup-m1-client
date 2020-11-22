//
// Created by leonardo on 22/11/20.
//

#include "scheduler.h"
#include "tools.h"
#include "message.h"
#include "tlv_view.h"


scheduler::scheduler(std::shared_ptr<connection> connection_ptr, std::shared_ptr<directory::dir2> dir_ptr)
        : connection_ptr_{std::move(connection_ptr)}, dir_ptr_{std::move(dir_ptr)} {}

std::shared_ptr<scheduler>
scheduler::get_instance(std::shared_ptr<connection> connection_ptr, std::shared_ptr<directory::dir2> dir_ptr) {
    return std::shared_ptr<scheduler>(new scheduler{std::move(connection_ptr), std::move(dir_ptr)});
}

void scheduler::schedule_sync() {
    communication::message sync_msg{communication::MESSAGE_TYPE::SYNC};
    sync_msg.add_TLV(communication::TLV_TYPE::END);
    this->connection_ptr_->async_write(sync_msg, [this, sync_msg]() {
        this->connection_ptr_->async_read([this, sync_msg](communication::message const &response_msg) {
            this->handle_sync(sync_msg, response_msg);
        });
    });
}

void scheduler::schedule_create(boost::filesystem::path const &relative_path, std::string const &digest) {
    std::string sign = tools::create_sign(relative_path, digest);
    auto request_msg = communication::message{communication::MESSAGE_TYPE::CREATE};
    request_msg.add_TLV(communication::TLV_TYPE::ITEM, sign.size(), sign.c_str());
    request_msg.add_TLV(communication::TLV_TYPE::FILE, relative_path);
    request_msg.add_TLV(communication::TLV_TYPE::END);
    directory::resource rsrc = directory::resource{
            boost::indeterminate,
            false,
            digest
    };
    this->dir_ptr_->insert_or_assign(relative_path, rsrc);
    std::cout << "Scheduling CREATE for: " << relative_path << ":\n\t" << rsrc;
    this->connection_ptr_->async_write(request_msg, [this, relative_path, request_msg]() {
        this->connection_ptr_->async_read(
                [this, relative_path, request_msg](communication::message const &response_msg) {
                    this->handle_create(relative_path, request_msg, response_msg);
                });
    });
}

void scheduler::schedule_update(boost::filesystem::path const &relative_path, std::string const &digest) {
    std::string sign = tools::create_sign(relative_path, digest);
    auto request_msg = communication::message{communication::MESSAGE_TYPE::UPDATE};
    request_msg.add_TLV(communication::TLV_TYPE::ITEM, sign.size(), sign.c_str());
    request_msg.add_TLV(communication::TLV_TYPE::FILE, relative_path);
    request_msg.add_TLV(communication::TLV_TYPE::END);
    directory::resource rsrc = directory::resource{
            boost::indeterminate,
            true,
            digest
    };
    this->dir_ptr_->insert_or_assign(relative_path, rsrc);
    std::cout << "Scheduling UPDATE for: " << relative_path << ":\n\t" << rsrc;

    this->connection_ptr_->async_write(request_msg, [this, relative_path, request_msg]() {
        this->connection_ptr_->async_read(
                [this, relative_path, request_msg](communication::message const &response_msg) {
                    this->handle_update(relative_path, request_msg, response_msg);
                });
    });
}

void scheduler::schedule_delete(boost::filesystem::path const &relative_path, std::string const &digest) {
    auto request_msg = communication::message{communication::MESSAGE_TYPE::DELETE};
    std::string sign = tools::create_sign(relative_path, digest);
    request_msg.add_TLV(communication::TLV_TYPE::ITEM, sign.size(), sign.c_str());
    request_msg.add_TLV(communication::TLV_TYPE::END);
    directory::resource rsrc = directory::resource{
            boost::indeterminate,
            true,
            digest
    };
    this->dir_ptr_->insert_or_assign(relative_path, rsrc);
    std::cout << "Scheduling DELETE for: " << relative_path << ":\n\t" << rsrc;
    this->connection_ptr_->async_write(request_msg, [this, relative_path, request_msg]() {
        this->connection_ptr_->async_read(
                [this, relative_path, request_msg](communication::message const &response_msg) {
                    this->handle_delete(relative_path, request_msg, response_msg);
                });
    });
}

void scheduler::handle_sync(communication::message const &request_msg, communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    communication::MESSAGE_TYPE c_msg_type = request_msg.get_msg_type();
    communication::MESSAGE_TYPE s_msg_type = response_msg.get_msg_type();
    if (c_msg_type != s_msg_type || !s_view.next_tlv() || s_view.get_tlv_type() == communication::TLV_TYPE::ERROR) {
        std::cerr << "Failed to sync server state" << std::endl;
        std::exit(-1);
    }
    do {
        if (s_view.get_tlv_type() == communication::TLV_TYPE::ITEM) {
            std::string s_sign{s_view.cbegin(), s_view.cend()};
            auto sign_parts = tools::split_sign(s_sign);
            boost::filesystem::path const &relative_path = sign_parts[0];
            std::string digest = sign_parts[1];
            if (!this->dir_ptr_->contains(relative_path)) {
                this->schedule_delete(relative_path, digest);
            }
            else {
                auto rsrc = this->dir_ptr_->rsrc(relative_path).value();
                if (rsrc.digest() != digest) this->schedule_update(relative_path, digest);
                else this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(true));
            }
        }
    } while (s_view.next_tlv());

    auto s_dir_ptr = directory::dir2::get_instance("S_DIR");

    this->dir_ptr_->for_each([this, &s_dir_ptr](std::pair<boost::filesystem::path, directory::resource> const &pair) {
        if (!s_dir_ptr->contains(pair.first)) {
            this->schedule_create(pair.first, pair.second.digest());
        }
    });
}

void scheduler::handle_create(boost::filesystem::path const &relative_path,
                              communication::message const &request_msg,
                              communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
    if (response_msg.get_msg_type() == communication::MESSAGE_TYPE::CREATE &&
        s_view.next_tlv() &&
        s_view.get_tlv_type() == communication::TLV_TYPE::ITEM) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(true));
    } else {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
    }
}

void scheduler::handle_update(boost::filesystem::path const &relative_path,
                              communication::message const &request_msg,
                              communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
    this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(
            response_msg.get_msg_type() == communication::MESSAGE_TYPE::UPDATE &&
            s_view.next_tlv() &&
            s_view.get_tlv_type() == communication::TLV_TYPE::ITEM
    ));
//    if (response_msg.get_msg_type() == communication::MESSAGE_TYPE::UPDATE &&
//        s_view.next_tlv() &&
//        s_view.get_tlv_type() == communication::TLV_TYPE::ITEM) {
//        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true));
//    } else {
//        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
//    }
}

void scheduler::handle_delete(boost::filesystem::path const &relative_path,
                              communication::message const &request_msg,
                              communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
    if (response_msg.get_msg_type() == communication::MESSAGE_TYPE::DELETE &&
        s_view.next_tlv() &&
        s_view.get_tlv_type() == communication::TLV_TYPE::ITEM) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(false));
    } else {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
    }
}