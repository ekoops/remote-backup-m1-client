#include <boost/function.hpp>
#include "scheduler.h"
#include "tools.h"
#include "message.h"
#include "tlv_view.h"
#include "f_message.h"

namespace fs = boost::filesystem;

/**
 * Construct a scheduler instance for a given watched directory.
 * The scheduler instance will use the connection abstraction
 * to communicate with the server and a thread pool for constructing
 * in parallel multiple request message
 *
 * @param io_context io_context
 * @param dir_ptr the watched directory std::shared_ptr
 * @param connection_ptr the connection std::shared_ptr
 * @param thread_pool_size the thread pool size
 * @return a new constructed scheduler instance
 */
scheduler::scheduler(
        boost::asio::io_context &io,
        std::shared_ptr<directory::dir> dir_ptr,
        std::shared_ptr<connection> connection_ptr,
        size_t thread_pool_size
) : dir_ptr_{std::move(dir_ptr)},
    connection_ptr_{std::move(connection_ptr)},
    io_{io},
    ex_work_guard_{boost::asio::make_work_guard(io_)} {
    this->thread_pool_.reserve(thread_pool_size);

    for (int i = 0; i < thread_pool_size; i++) {
        this->thread_pool_.emplace_back(
                boost::bind(&boost::asio::io_context::run, &this->io_),
                std::ref(this->io_)
        );
    }
}

/**
 * Construct a scheduler instance std::shared_ptr for a given watched directory.
 * The scheduler instance will use the connection abstraction
 * to communicate with the server and a thread pool for constructing
 * in parallel multiple request message
 *
 * @param io_context io_context
 * @param dir_ptr the watched directory std::shared_ptr
 * @param connection_ptr the connection std::shared_ptr
 * @param thread_pool_size the thread pool size
 * @return a new constructed scheduler instance std::shared_ptr
 */
std::shared_ptr<scheduler> scheduler::get_instance(
        boost::asio::io_context &io,
        std::shared_ptr<directory::dir> dir_ptr,
        std::shared_ptr<connection> connection_ptr,
        size_t thread_pool_size
) {
    return std::shared_ptr<scheduler>(new scheduler{
            io,
            std::move(dir_ptr),
            std::move(connection_ptr),
            thread_pool_size
    });
}


/**
 * Allow to handle SYNC message server response
 *
 * @param response an optional containing the eventual server response
 * @return void
 */
void scheduler::handle_sync(std::optional<communication::message> const &response) {
    if (!response) std::exit(EXIT_FAILURE);

    communication::message const &response_msg = response.value();

    communication::tlv_view s_view{response_msg};
    communication::MESSAGE_TYPE s_msg_type = response_msg.msg_type();
    if (s_msg_type != communication::MESSAGE_TYPE::SYNC ||
        !s_view.next_tlv() ||
        s_view.tlv_type() == communication::TLV_TYPE::ERROR) {
        std::cerr << "Failed to sync server state" << std::endl;
        std::exit(-1);
    }

    auto s_dir_ptr = directory::dir::get_instance("S_DIR");

    // Checking for server elements that should be deleted or updated
    do {
        if (s_view.tlv_type() == communication::TLV_TYPE::ITEM) {
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
                this->erase(relative_path, digest);
            } else {
                auto rsrc = this->dir_ptr_->rsrc(relative_path).value();
                if (rsrc.digest() != digest) this->update(relative_path, digest);
                else this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(true));
            }
        }
    } while (s_view.next_tlv());

    // Checking for server elements that should be created
    this->dir_ptr_->for_each([this, &s_dir_ptr](std::pair<fs::path, directory::resource> const &pair) {
        if (!s_dir_ptr->contains(pair.first)) {
            this->create(pair.first, pair.second.digest());
        }
    });
}

/**
 * Allow to handle CREATE message server response
 *
 * @param relative_path the relative path of the file related to the CREATE message
 * @param sign the sign of the file related to the CREATE message
 * @param response an optional containing the eventual server response
 * @return void
 */
void scheduler::handle_create(fs::path const &relative_path,
                              std::string const &sign,
                              std::optional<communication::message> const &response) {
    auto rsrc_opt = this->dir_ptr_->rsrc(relative_path);
    if (!rsrc_opt) std::exit(EXIT_FAILURE);
    directory::resource rsrc = rsrc_opt.value();

    if (!response) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
        return;
    }
    communication::message const &response_msg = response.value();
    communication::tlv_view s_view{response_msg};
    if (response_msg.msg_type() == communication::MESSAGE_TYPE::CREATE &&
        s_view.next_tlv() &&
        s_view.tlv_type() == communication::TLV_TYPE::ITEM &&
        sign == std::string{s_view.cbegin(), s_view.cend()} &&
        s_view.next_tlv() &&
        s_view.tlv_type() == communication::TLV_TYPE::OK) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(true));
    } else {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
    }
}

/**
 * Allow to handle UPDATE message server response
 *
 * @param relative_path the relative path of the file related to the UPDATE message
 * @param sign the sign of the file related to the UPDATE message
 * @param response an optional containing the eventual server response
 * @return void
 */
void scheduler::handle_update(fs::path const &relative_path,
                              std::string const &sign,
                              std::optional<communication::message> const &response) {
    auto rsrc_opt = this->dir_ptr_->rsrc(relative_path);
    if (!rsrc_opt) std::exit(EXIT_FAILURE);
    directory::resource rsrc = rsrc_opt.value();
    if (!response) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
        return;
    }
    communication::message const &response_msg = response.value();
    communication::tlv_view s_view{response_msg};
    this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(
            response_msg.msg_type() == communication::MESSAGE_TYPE::UPDATE &&
            s_view.next_tlv() &&
            s_view.tlv_type() == communication::TLV_TYPE::ITEM &&
            sign == std::string{s_view.cbegin(), s_view.cend()} &&
            s_view.next_tlv() &&
            s_view.tlv_type() == communication::TLV_TYPE::OK
    ));
}

/**
 * Allow to handle ERASE message server response
 *
 * @param relative_path the relative path of the file related to the ERASE message
 * @param sign the sign of the file related to the ERASE message
 * @param response an optional containing the eventual server response
 * @return void
 */
void scheduler::handle_erase(fs::path const &relative_path,
                             std::string const &sign,
                             std::optional<communication::message> const &response) {
    auto rsrc_opt = this->dir_ptr_->rsrc(relative_path);
    if (!rsrc_opt) std::exit(EXIT_FAILURE);
    directory::resource rsrc = rsrc_opt.value();
    if (!response) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
        return;
    }
    communication::message const &response_msg = response.value();
    communication::tlv_view s_view{response_msg};
    if (response_msg.msg_type() == communication::MESSAGE_TYPE::ERASE &&
        s_view.next_tlv() &&
        s_view.tlv_type() == communication::TLV_TYPE::ITEM &&
        sign == std::string{s_view.cbegin(), s_view.cend()} &&
        s_view.next_tlv() &&
        s_view.tlv_type() == communication::TLV_TYPE::OK) {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(true).exist_on_server(false));
    } else {
        this->dir_ptr_->insert_or_assign(relative_path, rsrc.synced(false));
    }
}

/**
 * Allow to schedule a SYNC operation through the associated connection
 *
 * @return void
 */
void scheduler::sync() {
    communication::message request_msg{communication::MESSAGE_TYPE::SYNC};
    request_msg.add_TLV(communication::TLV_TYPE::END);
    std::cout << "Scheduling SYNC..." << std::endl;

    auto response = this->connection_ptr_->sync_post(request_msg);
    this->handle_sync(response);
}

/**
 * Allow to schedule a CREATE operation through the associated connection
 *
 * @param relative_path the relative path of the file that has to be created
 * @param sign the sign of the file that has to be created
 *
 * @return void
 */
void scheduler::create(fs::path const &relative_path, std::string const &digest) {
    boost::asio::post(this->io_, [this, relative_path, digest]() {
        directory::resource rsrc = directory::resource{
                boost::indeterminate,
                false,
                digest
        };
        std::ostringstream oss;
        oss << "Scheduling CREATE for: " << relative_path << ":\n\t" << rsrc;
        std::cout << oss.str();
        this->dir_ptr_->insert_or_assign(relative_path, rsrc);

        std::string sign = tools::create_sign(relative_path, digest);
        auto f_msg = communication::f_message::get_instance(
                communication::MESSAGE_TYPE::CREATE,
                this->dir_ptr_->path() / relative_path,
                sign
        );

        this->connection_ptr_->post(
                f_msg,
                boost::asio::bind_executor(
                        this->io_,
                        boost::bind(
                                &scheduler::handle_create,
                                this,
                                relative_path,
                                sign,
                                boost::placeholders::_1
                        )
                )
        );
    });
}

/**
 * Allow to schedule a UPDATE operation through the associated connection
 *
 * @param relative_path the relative path of the file that has to be updated
 * @param sign the sign of the file that has to be updated
 *
 * @return void
 */
void scheduler::update(fs::path const &relative_path, std::string const &digest) {
    boost::asio::post(this->io_, [this, relative_path, digest]() {
        directory::resource rsrc = directory::resource{
                boost::indeterminate,
                true,
                digest
        };
        std::ostringstream oss;
        oss << "Scheduling UPDATE for: " << relative_path << ":\n\t" << rsrc;
        std::cout << oss.str();
        this->dir_ptr_->insert_or_assign(relative_path, rsrc);

        std::string sign = tools::create_sign(relative_path, digest);
        auto f_msg = communication::f_message::get_instance(
                communication::MESSAGE_TYPE::UPDATE,
                this->dir_ptr_->path() / relative_path,
                sign
        );

        this->connection_ptr_->post(
                f_msg,
                boost::asio::bind_executor(
                        this->io_,
                        boost::bind(
                                &scheduler::handle_update,
                                this,
                                relative_path,
                                sign,
                                boost::placeholders::_1
                        )
                )
        );
    });
}

/**
 * Allow to schedule a ERASE operation through the associated connection
 *
 * @param relative_path the relative path of the file that has to be erased
 * @param sign the sign of the file that has to be erased
 *
 * @return void
 */
void scheduler::erase(fs::path const &relative_path, std::string const &digest) {
    boost::asio::post(this->io_, [this, relative_path, digest]() {
        directory::resource rsrc = directory::resource{
                boost::indeterminate,
                true,
                digest
        };
        std::ostringstream oss;
        oss << "Scheduling ERASE for: " << relative_path << ":\n\t" << rsrc;
        std::cout << oss.str();
        this->dir_ptr_->insert_or_assign(relative_path, rsrc);

        std::string sign = tools::create_sign(relative_path, digest);
        communication::message request_msg{communication::MESSAGE_TYPE::ERASE};
        request_msg.add_TLV(communication::TLV_TYPE::ITEM, sign.size(), sign.c_str());
        request_msg.add_TLV(communication::TLV_TYPE::END);
        this->connection_ptr_->post(
                request_msg,
                boost::asio::bind_executor(
                        this->io_,
                        boost::bind(
                                &scheduler::handle_erase,
                                this,
                                relative_path,
                                sign,
                                boost::placeholders::_1
                        )
                )
        );
    });
}

void scheduler::close() {
    for (auto &t : this->thread_pool_) if (t.joinable()) t.join();
}