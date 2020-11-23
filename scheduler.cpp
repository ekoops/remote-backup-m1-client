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



bool scheduler::try_auth(std::string const &username, std::string const &password) {
    communication::message auth_msg{communication::MESSAGE_TYPE::AUTH};
    auth_msg.add_TLV(communication::TLV_TYPE::USRN, username.size(), username.c_str());
    auth_msg.add_TLV(communication::TLV_TYPE::PSWD, password.size(), password.c_str());
    auth_msg.add_TLV(communication::TLV_TYPE::END);

    std::optional<communication::message> msg;
    if (this->connection_ptr_->write(auth_msg) && (msg = this->connection_ptr_->read())) {
        communication::tlv_view view{msg.value()};
        return view.next_tlv() && view.get_tlv_type() == communication::TLV_TYPE::OK;
    }
    return false;
}

void scheduler::fail_callback(std::shared_ptr<directory::dir2> const &dir_ptr_,
                              boost::filesystem::path const &relative_path) {
    auto rsrc = dir_ptr_->rsrc(relative_path);
    if (rsrc) dir_ptr_->insert_or_assign(relative_path, rsrc.value().synced(false));
}

void scheduler::schedule_sync() {
    communication::message sync_msg{communication::MESSAGE_TYPE::SYNC};
    sync_msg.add_TLV(communication::TLV_TYPE::END);
    std::cout << "Scheduling SYNC..." << std::endl;
    this->connection_ptr_->async_write(sync_msg, [this]() {
        this->connection_ptr_->async_read([this](communication::message const &response_msg) {
            this->handle_sync(response_msg);
        }, []() { std::exit(-1); });
    }, []() { std::exit(-1); });
}

void scheduler::schedule_create(boost::filesystem::path const &relative_path, std::string const &digest) {
    std::string sign = tools::create_sign(relative_path, digest);
    auto request_msg = communication::message{communication::MESSAGE_TYPE::CREATE};
    request_msg.add_TLV(communication::TLV_TYPE::ITEM, sign.size(), sign.c_str());
    request_msg.add_TLV(communication::TLV_TYPE::FILE, this->dir_ptr_->path() / relative_path);
    request_msg.add_TLV(communication::TLV_TYPE::END);
    directory::resource rsrc = directory::resource{
            boost::indeterminate,
            false,
            digest
    };
    this->dir_ptr_->insert_or_assign(relative_path, rsrc);
    std::cout << "Scheduling CREATE for: " << relative_path << ":\n\t" << rsrc;

    this->connection_ptr_->async_write(request_msg, [this, relative_path, sign]() {
        this->connection_ptr_->async_read([this, relative_path, sign](communication::message const &response_msg) {
            this->handle_create(relative_path, sign, response_msg);
        }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
    }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
}

void scheduler::schedule_update(boost::filesystem::path const &relative_path, std::string const &digest) {
    std::string sign = tools::create_sign(relative_path, digest);
    auto request_msg = communication::message{communication::MESSAGE_TYPE::UPDATE};
    request_msg.add_TLV(communication::TLV_TYPE::ITEM, sign.size(), sign.c_str());
    request_msg.add_TLV(communication::TLV_TYPE::FILE, this->dir_ptr_->path() / relative_path);
    request_msg.add_TLV(communication::TLV_TYPE::END);
    directory::resource rsrc = directory::resource{
            boost::indeterminate,
            true,
            digest
    };
    this->dir_ptr_->insert_or_assign(relative_path, rsrc);
    std::cout << "Scheduling UPDATE for: " << relative_path << ":\n\t" << rsrc;

    this->connection_ptr_->async_write(request_msg, [this, relative_path, sign]() {
        this->connection_ptr_->async_read([this, relative_path, sign](communication::message const &response_msg) {
            this->handle_update(relative_path, sign, response_msg);
        }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
    }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
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
//    auto fail_callback = [this, relative_path]() {
//        auto rsrc = this->dir_ptr_->rsrc(relative_path);
//        if (rsrc) this->dir_ptr_->insert_or_assign(relative_path, rsrc.value().synced(false));
//    };
    this->connection_ptr_->async_write(request_msg, [this, relative_path, sign]() {
        this->connection_ptr_->async_read([this, relative_path, sign](communication::message const &response_msg) {
            this->handle_delete(relative_path, sign, response_msg);
        }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
    }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
}

void scheduler::handle_sync(communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    communication::MESSAGE_TYPE s_msg_type = response_msg.get_msg_type();
    if (s_msg_type != communication::MESSAGE_TYPE::SYNC ||
        !s_view.next_tlv() ||
        s_view.get_tlv_type() == communication::TLV_TYPE::ERROR) {
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
            } else {
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
    std::cout << "SYNC ended" << std::endl;
}

void scheduler::handle_create(boost::filesystem::path const &relative_path,
                              std::string const &sign,
                              communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
    if (response_msg.get_msg_type() == communication::MESSAGE_TYPE::CREATE &&
        s_view.next_tlv() &&
        s_view.get_tlv_type() == communication::TLV_TYPE::ITEM &&
        sign == std::string{s_view.cbegin(), s_view.cend()}) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(true));
    } else {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
    }
}

void scheduler::handle_update(boost::filesystem::path const &relative_path,
                              std::string const &sign,
                              communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
    this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(
            response_msg.get_msg_type() == communication::MESSAGE_TYPE::UPDATE &&
            s_view.next_tlv() &&
            s_view.get_tlv_type() == communication::TLV_TYPE::ITEM &&
            sign == std::string{s_view.cbegin(), s_view.cend()}
    ));
//    if (response_msg.get_msg_type() == communication::MESSAGE_TYPE::UPDATE &&
//        s_view.next_tlv() &&
//        s_view.get_tlv_type() == communication::TLV_TYPE::ITEM &&
//        sign == std::string{s_view.cbegin(), s_view.cend()}
//        ) {
//        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true));
//    } else {
//        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
//    }
}

//req CREATE ITEM LENGTH PATH\x00DIGEST FILE LENGTH ....
//res CREATE ITEM LENGTH PATH\x00DIGEST | CREATE ERROR 0
//req UPDATE ITEM LENGTH PATH\x00DIGEST FILE LENGTH ....
//res UPDATE ITEM LENGTH PATH\x00DIGEST | UPDATE ERROR 0
//req DELETE ITEM LENGTH PATH\x00DIGEST FILE LENGTH ....
//res DELETE ITEM LENGTH PATH\x00DIGEST | DELETE ERROR 0
//req SYNC
//res SYNC ITEM LENGTH PATH\x00DIGEST ITEM ... | SYNC ERROR 0
//req AUTH USRN LENGTH username PSWD LENGTH password
//res AUTH OK 0 | AUTH ERROR 0
void scheduler::handle_delete(boost::filesystem::path const &relative_path,
                              std::string const &sign,
                              communication::message const &response_msg) {

    communication::tlv_view s_view{response_msg};
    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
    if (response_msg.get_msg_type() == communication::MESSAGE_TYPE::DELETE &&
        s_view.next_tlv() &&
        s_view.get_tlv_type() == communication::TLV_TYPE::ITEM &&
        sign == std::string{s_view.cbegin(), s_view.cend()}) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(false));
    } else {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
    }
}