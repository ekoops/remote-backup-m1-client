#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>
#include "file_watcher.h"
#include "message.h"
#include "connection.h"

#define DELAY_MS 5000
#define THREAD_POOL_SIZE 8

namespace fs = boost::filesystem;


bool login(const std::shared_ptr<scheduler> &scheduler) {
    std::string username, password;
//    boost::regex username_regex{"[a-z][a-z\\d_\\.]{7, 15}"};
//    boost::regex password_regex{"[a-zA-Z0-9\\d\\-\\.@$!%*?&]{8, 16}"};
    boost::regex username_regex{".*"};
    boost::regex password_regex{".*"};
    int field_attempts = 3;
    int general_attempts = 3;

    while (general_attempts) {
        std::cout << "Insert your username:" << std::endl;
        while (!(std::cin >> username) || !boost::regex_match(username, username_regex)) {
            std::cin.clear();
            std::cout << "Failed to get username. Try again (attempts left " << --field_attempts << ")."
                      << std::endl;
            if (!field_attempts) return false;
        }
        field_attempts = 0;
        std::cout << "Insert your password:" << std::endl;
        while (!(std::cin >> password) || !boost::regex_match(password, password_regex)) {
            std::cin.clear();
            std::cout << "Failed to get password. Try again (attempts left " << --field_attempts << ")."
                      << std::endl;
            if (!field_attempts) return false;
        }
        if (scheduler->try_auth(username, password)) return true;
        else std::cout << "Authentication failed (attempts left " << --general_attempts << ")." << std::endl;
    }
    return false;
}

int main(int argc, char const *const argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ./client <path_to_watch> <hostname> <port>" << std::endl;
        return 1;
    }
    try {
        // canonical returns the absolute path resolving symbolic link, dot and dot-dot
        // it also checks the existence
        fs::path path_to_watch{fs::canonical(argv[1])};
        if (!fs::is_directory(path_to_watch)) {
            std::cerr << "<path_to_watch> provided is not a directory" << std::endl;
            return 2;
        }
        std::string hostname{argv[2]};
        std::string port{argv[3]};

        // Constructing an abstraction for the watched directory
        auto watched_dir_ptr = directory::dir::get_instance(path_to_watch, true);

        // Setting up the SSL connection
        boost::asio::io_context io_context;
        boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23};
//        ctx.load_verify_file("ca.pem");
        auto connection_ptr = connection::get_instance(io_context, ctx, THREAD_POOL_SIZE);
        // Constructing an abstraction for scheduling task and managing communication
        // with server through the connection
        auto scheduler_ptr = scheduler::get_instance(watched_dir_ptr, connection_ptr);

        connection_ptr->connect(hostname, port);
        if (!login(scheduler_ptr)) {
            std::cerr << "Authentication failed" << std::endl;
            return -3;
        }

        // Constructing an abstraction for monitoring the filesystem and scheduling
        // server sinchronizations through scheduler
        file_watcher fw {watched_dir_ptr, scheduler_ptr, std::chrono::milliseconds{DELAY_MS}};
        fw.start();
        connection_ptr->join_threads();
    }
    catch (fs::filesystem_error &e) {
        std::cerr << "Filesystem error from " << e.what() << std::endl;
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