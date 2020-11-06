#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/regex.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>
#include <netinet/in.h>
#include "file_watcher.h"
#include "dir.h"
#include "op.h"
#include "tlv_view.h"
#include "tools.h"

#define WRITE_MASK 4 << 6
#define DELAY_MS 5000

std::ostream &operator<<(std::ostream &os, const std::array<uint32_t, 4> &arr) {
    copy(arr.cbegin(), arr.cend(), std::ostream_iterator<uint32_t>(os, " "));
    return os;
}

namespace operation {
    std::size_t hash_value(op const &operation) {
        boost::hash<std::vector<uint8_t>> hasher;
        return hasher(*operation.raw_op_);
    }
}


bool f(operation::tlv_view const &c_view, operation::tlv_view const &s_view,
       std::function<bool(std::vector<std::string> const &)> lambda) {
    if (s_view.get_tlv_type() == operation::TLV_TYPE::ITEM) {
        std::string c_sign{c_view.cbegin(), c_view.cend()};
        std::string s_sign{s_view.cbegin(), s_view.cend()};
        if (s_sign != c_sign) {
            std::cerr << "Server manages a different signature"
                      << "\nC:" << c_sign
                      << "\nS:" << s_sign << std::endl;
            return false;
        } else {
            std::istringstream oss{s_sign};
            std::string temp;
            std::vector<std::string> results(2);
            while (std::getline(oss, temp, '\x00')) results.push_back(temp);
            return lambda(results);
        }
    } else {
        std::cerr << "Wrong server response format" << std::endl;
        return false;
    }
}

void op_handler(boost::asio::ip::tcp::socket &socket, std::shared_ptr<operation::operation_queue> &poq,
                std::shared_ptr<directory::dir> const &dir) {
    operation::op client_op = poq->pop();
    client_op.write_on_socket(socket);
    operation::op server_op = operation::op::read_from_socket(socket);
    // the server and the client operation types are the same
    operation::OPERATION_TYPE c_op_type = client_op.get_op_type();
    operation::OPERATION_TYPE s_op_type = server_op.get_op_type();
    operation::tlv_view c_view{client_op};
    // reading response from the server...
    operation::tlv_view s_view{server_op};

    if (s_op_type != c_op_type) {
        std::cerr << "Wrong server response" << std::endl;
        std::exit(-1);
    }
    if (!s_view.next_tlv()) {
        std::cerr << "Failed to read server message" << std::endl;
        std::exit(-1);
    }
    auto s_tlv_type = s_view.get_tlv_type();
    if (s_tlv_type == operation::TLV_TYPE::ERROR) {
        std::cerr << "Operation failed: " << std::string{s_view.begin(), s_view.end()} << std::endl;
        std::exit(-2);
    }

    c_view.next_tlv();
    if (c_op_type == operation::OPERATION_TYPE::CREATE) {
        if (!f(c_view, s_view, [&dir](auto results) {
            return dir->insert(results[1], results[2]);
        })) {
            std::exit(-1);
        }
    } else if (s_op_type == operation::OPERATION_TYPE::UPDATE) {
        if (!f(c_view, s_view, [&dir](auto results) {
            return dir->update(results[1], results[2]);
        })) {
            std::exit(-1);
        }
    }
    else if (s_op_type == operation::OPERATION_TYPE::DELETE) {
        if (!f(c_view, s_view, [&dir](auto results) {
            return dir->erase(results[1]);
        })) {
            std::exit(-1);
        }
    } else if (s_op_type == operation::OPERATION_TYPE::SYNC) {

        boost::filesystem::path path_from_server;
        //TODO obtain server_directory from server

        std::shared_ptr<directory::dir> server_dir = directory::dir::get_instance(path_from_server);
        for (auto &de : *(server_dir->get_content())) {
            std::pair<bool, bool> pair = this->watched_dir_->contains_and_match(de.first, de.second);
            // if doesn't exist
            if (!pair.first) {
                auto op = operation::op{operation::OPERATION_TYPE::DELETE};
                op.add_TLV(operation::TLV_TYPE::PATH, sizeof(de.first.generic_path()), de.first.generic_path().c_str());
                this->poq_->push(op);
                // if updated
            } else if (pair.second) {
                auto op = operation::op{operation::OPERATION_TYPE::UPDATE};
                op.add_TLV(operation::TLV_TYPE::PATH, sizeof(de.first.generic_path()), de.first.generic_path().c_str());
                this->poq_->push(op);
            }
        }
        this->watched_dir_->for_each_if([&server_dir](boost::filesystem::path const &path) {
            return !server_dir->contains(path);
        }, [this](std::pair<boost::filesystem::path, std::string> const &pair) {
            auto op = operation::op{operation::OPERATION_TYPE::CREATE};
            op.add_TLV(operation::TLV_TYPE::PATH, sizeof(pair.first.generic_path()), pair.first.generic_path().c_str());
            this->poq_->push(op);
        });

    } else {

//            auto dir = directory::dir::get_instance("SERVER_DIR");
//            dir->insert()
    }


}
// SYNC ITEM LENGTH {PATH1 \nullcharacter hash}

// AUTH USER LENGTH VALUE_USERNAME PASSWD LENGTH VALUE_PASSWORD TYPE_STOP
bool authenticate(boost::asio::ip::tcp::socket &socket) {
    std::string username, password;
    boost::regex username_regex{"[a-z][a-z\\d_\\.]{7, 15}"};
    boost::regex password_regex{"[a-zA-Z0-9\\d\\-\\.@$!%*?&]{8, 16}"};
    int field_attempts = 3;
    int general_attempts = 3;

    while (general_attempts) {
        std::cout << "Insert your username:" << std::endl;
        while (!(std::cin >> username) || !boost::regex_match(username, username_regex)) {
            std::cin.clear();
            std::cout << "Failed to get username. Try again (attempts left " << --field_attempts << ")." << std::endl;
            if (!field_attempts) return false;
        }
        field_attempts = 0;
        std::cout << "Insert your password:" << std::endl;
        while (!(std::cin >> password) || !boost::regex_match(password, password_regex)) {
            std::cin.clear();
            std::cout << "Failed to get password. Try again (attempts left " << --field_attempts << ")." << std::endl;
            if (!field_attempts) return false;
        }
        operation::op auth_op{operation::OPERATION_TYPE::AUTH};
        auth_op.add_TLV(operation::TLV_TYPE::USRN, username.size(), username.c_str());
        auth_op.add_TLV(operation::TLV_TYPE::PSWD, password.size(), password.c_str());
        auth_op.add_TLV(operation::TLV_TYPE::END);
        auth_op.write_on_socket(socket);
        operation::op op = operation::op::read_from_socket(socket);
        operation::tlv_view view{op};

        if (view.next_tlv() && view.get_tlv_type() == operation::TLV_TYPE::OK) return true;
        else std::cout << "Authentication failed (attempts left " << --general_attempts << ")." << std::endl;
    }
    return false;
}


int main(int argc, char const *const argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ./name path_to_watch hostname portno" << std::endl;
        return -1;
    }
    boost::filesystem::path path_to_watch{boost::filesystem::canonical(argv[1])};
    std::string hostname{argv[2]};
    std::string portno{argv[3]};
    // Trying to parse port number
    try {
        // Checking path existance and permissions...
        if (!exists(path_to_watch) ||
            !is_directory(path_to_watch) ||
            (status(path_to_watch).permissions() & WRITE_MASK) != WRITE_MASK) {
            std::cerr << "Path provided doesn't exist is not a directory or permission denied" << std::endl;
            return -2;
        }
//        boost::asio::io_context io_context;
//        boost::asio::ip::tcp::resolver resolver{io_context};
//        boost::asio::ip::tcp::resolver::results_type endpoints =
//                resolver.resolve(hostname, portno); // equivalente a gethostbyname
//        boost::asio::ip::tcp::socket socket{io_context};
//        boost::asio::connect(socket, endpoints);
//
//
//        if (!authenticate(socket)) {
//            std::cerr << "Authentication failed" << std::endl;
//            return -3;
//        }
//
        auto poq = operation::operation_queue::get_instance();
        auto dir = directory::dir::get_instance(path_to_watch, true);

//        operation::op sync_op{operation::OPERATION_TYPE::SYNC};
//        sync_op.add_TLV(operation::TLV_TYPE::END);
//        poq->push(sync_op);
//
//
        file_watcher fw{dir, poq, std::chrono::milliseconds{DELAY_MS}};
//        boost::thread fw_thread{[](file_watcher &fw) {
//            fw.sync_watched_dir();
//            fw.start();
//        }, std::ref(fw)};
//        boost::thread op_thread{op_handler, std::ref(socket), std::ref(poq), std::cref(dir)};
//        fw_thread.join();
//        op_thread.join();



//        while (true) {
//            buf.fill(0);
//            boost::system::error_code error;
//            size_t len = socket.read_some(boost::asio::buffer(buf), error);
//            size_t last_index = strlen(buf.data()) - 1;
//            if (buf[last_index] == '\n') buf[last_index] = 0;
//            if (error == boost::asio::error::eof) break;
//            else if (error) throw boost::system::system_error(error);
//            socket.write_some(boost::asio::buffer(buf), error);
//            std::cout.write(buf.data(), strlen(buf.data())) << std::endl;
//        }
    }
    catch (boost::filesystem::filesystem_error &e) {
        std::cerr << "Filesystem error with code " << e.code() << ": " << e.what() << std::endl;
        return -4;
    }
    catch (boost::system::system_error &e) {
        std::cerr << "System error with code " << e.code() << ": " << e.what() << std::endl;
        return -5;
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return -6;
    }
    return 0;
}