#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/regex.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>
#include "file_watcher.h"
#include "message.h"
#include "connection.h"

#define DELAY_MS 5000

bool login(const std::shared_ptr<scheduler>& scheduler) {
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
        std::cerr << "Usage: ./name path_to_watch hostname portno" << std::endl;
        return -1;
    }
    try {
        // canonical returns the absolute path resolving symbolic link, dot and dot-dot
        // it also checks the existance
        boost::filesystem::path path_to_watch{boost::filesystem::canonical(argv[1])};
        std::string hostname{argv[2]};
        // Trying to parse port number
        std::string portno{argv[3]};
        // Checking path existance and permissions...
        if (!boost::filesystem::is_directory(path_to_watch)) {
            std::cerr << "Path provided is not a directory" << std::endl;
            return -2;
        }
        auto dir2 = directory::dir2::get_instance(path_to_watch, true);
        boost::asio::io_context io_context;
        auto conn = connection::get_instance(io_context);
        auto scheduler = scheduler::get_instance(conn, dir2);
        conn->connect(hostname, portno);
        if (!login(scheduler)) {
            std::cerr << "Authentication failed" << std::endl;
            return -3;
        }
        std::cout << "Authenticated" << std::endl;
        file_watcher fw{dir2, scheduler, std::chrono::milliseconds{DELAY_MS}};
        scheduler->schedule_sync();
//        fw.start();
        boost::thread fw_thread{[](file_watcher &fw) {
            fw.start();
        }, std::ref(fw)};
        fw_thread.join();
    }
    catch (boost::filesystem::filesystem_error &e) {
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