#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/regex.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread/thread.hpp>
#include <netinet/in.h>
#include <netdb.h>
#include "file_watcher.h"
#include "dir.h"
#include "op.h"
#include "tlv_view.h"

#define WRITE_MASK 4 << 6
#define DELAY_MS 5000

namespace operation {
    std::size_t hash_value(op const &operation) {
        boost::hash<std::vector<uint8_t>> hasher;
        return hasher(*operation.raw_op_);
    }
}


void op_handler(boost::asio::ip::tcp::socket &socket, std::shared_ptr<operation::operation_queue> &poq,
                std::shared_ptr<directory::dir> const &ld) {
    operation::op client_op = poq->pop();
    client_op.write_on_socket(socket);
    operation::op server_op = operation::op::read_from_socket(socket);
    operation::OPERATION_TYPE op_type = server_op.get_op_type();
    operation::tlv_view view {server_op};
    if (!view.next_tlv()) {
        std::cerr << "Failed to read server message" << std::endl;
        std::exit(-1);
    }
    else if (view.get_tlv_type() == operation::TLV_TYPE::ERROR) {
        std::cerr << "Operation failed: " << std::string {view.begin(), view.end()} << std::endl;
        std::exit(-2);
    }
    while (view.next_tlv()) {
        view.get_tlv_type();
        view.get_length();
        std::string s {view.begin(), view.end()};
    }

    if (op_type == operation::OPERATION_TYPE::CREATE) {

    } else if (op_type == operation::OPERATION_TYPE::UPDATE) {

    } else if (op_type == operation::OPERATION_TYPE::DELETE) {

    } else if (op_type == operation::OPERATION_TYPE::SYNC) {

    } else {

        auto dir = directory::dir::get_instance("SERVER_DIR");
        dir->insert()
    }
}
// SYNC PATH LENGTH {PATH1:hash}

// AUTH USER LENGTH VALUE_USERNAME PASSWD LENGTH VALUE_PASSWORD TYPE_STOP
bool authenticate(boost::asio::ip::tcp::socket &socket) {
    std::string username, password;
    int attempts = 0;

    std::cout << "Insert your username:" << std::endl;
    boost::regex username_regex{"[a-z][a-z\\d_\\.]{7, 15}"};
    boost::regex password_regex{"[a-zA-Z0-9\\d\\-\\.@$!%*?&]{8, 16}"};
    while (!(std::cin >> username) || !boost::regex_match(username, username_regex)) {
        std::cin.clear();
        attempts++;
        std::cout << "Failed to get username. Try again (attempts left " << 3 - attempts << ")." << std::endl;
        if (attempts == 3) return false;
    }
    attempts = 0;
    std::cout << "Insert your password:" << std::endl;
    while (!(std::cin >> password) || !boost::regex_match(password, password_regex)) {
        std::cin.clear();
        attempts++;
        std::cout << "Failed to get password. Try again (attempts left " << 3 - attempts << ")." << std::endl;
        if (attempts == 3) return false;
    }

    operation::op auth_op {operation::OPERATION_TYPE::AUTH};
    auth_op.add_TLV(operation::TLV_TYPE::USRN, username.size(), username.c_str());
    auth_op.add_TLV(operation::TLV_TYPE::PSWD, password.size(), password.c_str());
    auth_op.add_TLV(operation::TLV_TYPE::END);
    auth_op.write_on_socket(socket);
    operation::op op = operation::op::read_from_socket(socket);
    operation::tlv_view view {op};

    return view.next_tlv() && view.get_tlv_type() == operation::TLV_TYPE::OK;
}


int main(int argc, char const *const argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ./name path_to_watch hostname portno" << std::endl;
        return -1;
    }
    boost::filesystem::path path_to_watch{argv[1]};
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

        boost::asio::io_context io_context;
        boost::asio::ip::tcp::resolver resolver{io_context};
        boost::asio::ip::tcp::resolver::results_type endpoints =
                resolver.resolve(hostname, portno); // equivalente a gethostbyname
        boost::asio::ip::tcp::socket socket{io_context};
        boost::asio::connect(socket, endpoints);
        if (!authenticate(socket)) {
            std::cerr << "Authentication failed" << std::endl;
            return -3;
        }

        auto poq = operation::operation_queue::get_instance();
        auto dir = directory::dir::get_instance(path_to_watch, true);

        operation::op sync_op {operation::OPERATION_TYPE::SYNC};
        sync_op.add_TLV(operation::TLV_TYPE::END);
        poq->push(sync_op);


        file_watcher fw{dir, poq, std::chrono::milliseconds{DELAY_MS}};
        boost::thread fw_thread{[](file_watcher &fw) {
            fw.sync_watched_dir();
            fw.start();
        }, std::ref(fw)};
        boost::thread op_thread{op_handler, std::ref(socket), std::ref(poq), std::cref(dir)};
        fw_thread.join();
        op_thread.join();



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