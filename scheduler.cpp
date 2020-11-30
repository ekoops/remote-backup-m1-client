//
// Created by leonardo on 22/11/20.
//

#include "scheduler.h"
#include "tools.h"
#include "message.h"
#include "tlv_view.h"
#include "f_message.h"
#include <boost/function.hpp>

namespace fs = boost::filesystem;

scheduler::scheduler(std::shared_ptr<directory::dir> dir_ptr, std::shared_ptr<connection> connection_ptr)
        : dir_ptr_{std::move(dir_ptr)}, connection_ptr_{std::move(connection_ptr)} {}

std::shared_ptr<scheduler>
scheduler::get_instance(std::shared_ptr<directory::dir> dir_ptr, std::shared_ptr<connection> connection_ptr) {
    return std::shared_ptr<scheduler>(new scheduler{std::move(dir_ptr), std::move(connection_ptr)});
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

void scheduler::fail_callback(std::shared_ptr<directory::dir> const &dir_ptr_,
                              fs::path const &relative_path) {
    auto rsrc = dir_ptr_->rsrc(relative_path);
    if (rsrc) dir_ptr_->insert_or_assign(relative_path, rsrc.value().synced(false));
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

    auto s_dir_ptr = directory::dir::get_instance("S_DIR");

    // Checking for server elements that should be deleted or updated
    do {
        if (s_view.get_tlv_type() == communication::TLV_TYPE::ITEM) {
            std::string s_sign{s_view.cbegin(), s_view.cend()};
            auto splitted_sign = tools::split_sign(s_sign);
            fs::path const &relative_path = splitted_sign.first;
            std::string digest = splitted_sign.second;
            s_dir_ptr->insert_or_assign(relative_path, directory::resource{
                    boost::indeterminate, // unused field for server dir
                    true,   // unused field for server dir
                    digest
            });
            if (!this->dir_ptr_->contains(relative_path)) {
                this->schedule_delete(relative_path, digest);
            } else {
                auto rsrc = this->dir_ptr_->rsrc(relative_path).value();
                if (rsrc.digest() != digest) this->schedule_update(relative_path, digest);
                else this->dir_ptr_->insert_or_assign(relative_path,rsrc.synced(true).exist_on_server(true));
            }
        }
    } while (s_view.next_tlv());

    // Checking for server elements that should be created
    this->dir_ptr_->for_each([this, &s_dir_ptr](std::pair<fs::path, directory::resource> const &pair) {
        if (!s_dir_ptr->contains(pair.first)) {
            this->schedule_create(pair.first, pair.second.digest());
        }
    });
}

void scheduler::handle_create(fs::path const &relative_path,
                              std::string const &sign,
                              communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
    if (response_msg.get_msg_type() == communication::MESSAGE_TYPE::CREATE &&
        s_view.next_tlv() &&
        s_view.get_tlv_type() == communication::TLV_TYPE::ITEM &&
        sign == std::string{s_view.cbegin(), s_view.cend()} &&
        s_view.next_tlv() &&
        s_view.get_tlv_type() == communication::TLV_TYPE::OK) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(true));
    } else {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
    }
}

void scheduler::handle_update(fs::path const &relative_path,
                              std::string const &sign,
                              communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
    this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(
            response_msg.get_msg_type() == communication::MESSAGE_TYPE::UPDATE &&
            s_view.next_tlv() &&
            s_view.get_tlv_type() == communication::TLV_TYPE::ITEM &&
            sign == std::string{s_view.cbegin(), s_view.cend()} &&
            s_view.next_tlv() &&
            s_view.get_tlv_type() == communication::TLV_TYPE::OK
    ));
}

void scheduler::handle_delete(fs::path const &relative_path,
                              std::string const &sign,
                              communication::message const &response_msg) {
    communication::tlv_view s_view{response_msg};
    directory::resource rsrc = this->dir_ptr_->rsrc(relative_path).value();
    if (response_msg.get_msg_type() == communication::MESSAGE_TYPE::DELETE &&
        s_view.next_tlv() &&
        s_view.get_tlv_type() == communication::TLV_TYPE::ITEM &&
        sign == std::string{s_view.cbegin(), s_view.cend()} &&
        s_view.next_tlv() &&
        s_view.get_tlv_type() == communication::TLV_TYPE::OK) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(false));
    } else {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
    }
}

void scheduler::schedule_sync() {
    communication::message request_msg{communication::MESSAGE_TYPE::SYNC};
    request_msg.add_TLV(communication::TLV_TYPE::END);
    std::cout << "Scheduling SYNC..." << std::endl;

    this->connection_ptr_->post(request_msg, boost::bind(
            &scheduler::handle_sync,
            this,
            boost::placeholders::_1
    ), []() { std::exit(-1); });

//        this->connection_ptr_->async_write2(request_msg,[](communication::message const& response_message){
////        std::cout << response_message << std::endl;
//        } );
//    ASYNC VERSION
//    this->connection_ptr_->async_write(request_msg, [this]() {
//        this->connection_ptr_->async_read([this](communication::message const &response_msg) {
//            this->handle_sync(response_msg);
//        }, []() { std::exit(-1); });
//    }, []() { std::exit(-1); });
}


void scheduler::schedule_create(fs::path const &relative_path, std::string const &digest) {
    directory::resource rsrc = directory::resource{
            boost::indeterminate,
            false,
            digest
    };
    std::cout << "Scheduling CREATE for: " << relative_path << ":\n\t" << rsrc;
    this->dir_ptr_->insert_or_assign(relative_path, rsrc);

    std::string sign = tools::create_sign(relative_path, digest);
    auto f_msg = communication::f_message::get_instance(
            communication::MESSAGE_TYPE::CREATE,
            this->dir_ptr_->path() / relative_path,
            sign
    );

    this->connection_ptr_->post(f_msg, boost::bind(
            &scheduler::handle_create,
            this,
            relative_path,
            sign,
            boost::placeholders::_1
    ), boost::bind(&scheduler::fail_callback, this->dir_ptr_, relative_path));

//    this->connection_ptr_->async_write2(request_msg,[](communication::message const& response_message){
////        std::cout << response_message << std::endl;
//    } );

//    ASYNC VERSION
//    this->connection_ptr_->async_write(request_msg, [this, relative_path, sign]() {
//        this->connection_ptr_->async_read([this, relative_path, sign](communication::message const &response_msg) {
//            this->handle_create(relative_path, sign, response_msg);
//        }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
//    }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
}

void scheduler::schedule_update(fs::path const &relative_path, std::string const &digest) {
    directory::resource rsrc = directory::resource{
            boost::indeterminate,
            true,
            digest
    };
    std::cout << "Scheduling UPDATE for: " << relative_path << ":\n\t" << rsrc;
    this->dir_ptr_->insert_or_assign(relative_path, rsrc);

    std::string sign = tools::create_sign(relative_path, digest);
    auto f_msg = communication::f_message::get_instance(
            communication::MESSAGE_TYPE::UPDATE,
            this->dir_ptr_->path() / relative_path,
            sign
    );

    this->connection_ptr_->post(f_msg, boost::bind(
            &scheduler::handle_update,
            this,
            relative_path,
            sign,
            boost::placeholders::_1
    ), boost::bind(&scheduler::fail_callback, this->dir_ptr_, relative_path));


//    this->connection_ptr_->async_write2(request_msg,[](communication::message const& response_message){
////        std::cout << response_message << std::endl;
//    });

//    ASYNC VERSION
//    this->connection_ptr_->async_write(request_msg, [this, relative_path, sign]() {
//        this->connection_ptr_->async_read([this, relative_path, sign](communication::message const &response_msg) {
//            this->handle_update(relative_path, sign, response_msg);
//        }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
//    }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
}

void scheduler::schedule_delete(fs::path const &relative_path, std::string const &digest) {
    directory::resource rsrc = directory::resource{
            boost::indeterminate,
            true,
            digest
    };
    std::cout << "Scheduling DELETE for: " << relative_path << ":\n\t" << rsrc;
    this->dir_ptr_->insert_or_assign(relative_path, rsrc);

    std::string sign = tools::create_sign(relative_path, digest);
    communication::message request_msg{communication::MESSAGE_TYPE::DELETE};
    request_msg.add_TLV(communication::TLV_TYPE::ITEM, sign.size(), sign.c_str());
    request_msg.add_TLV(communication::TLV_TYPE::END);
    this->connection_ptr_->post(request_msg, boost::bind(
            &scheduler::handle_delete,
            this,
            relative_path,
            sign,
            boost::placeholders::_1
    ), boost::bind(&scheduler::fail_callback, this->dir_ptr_, relative_path));



//    this->connection_ptr_->async_write2(request_msg,[](communication::message const& response_message){
////        std::cout << response_message << std::endl;
//    } );

    //    ASYNC VERSION
//    this->connection_ptr_->async_write(request_msg, [this, relative_path, sign]() {
//        this->connection_ptr_->async_read([this, relative_path, sign](communication::message const &response_msg) {
//            this->handle_delete(relative_path, sign, response_msg);
//        }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
//    }, boost::bind(scheduler::fail_callback, this->dir_ptr_, relative_path));
}