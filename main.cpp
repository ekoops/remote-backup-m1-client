#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/regex.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread/thread.hpp>
#include <netinet/in.h>
#include <netdb.h>
#include "FileWatcher.h"
#include "LockedDirectory.h"
#include "Operation2.h"

#define WRITE_MASK 4 << 6
#define DELAY_MS 5000


void op_handler(boost::asio::ip::tcp::socket &socket, std::shared_ptr<PendingOperationsQueue> &poq,
                std::shared_ptr<LockedDirectory> const &ld) {

}

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
    Operation2 op2 {OPERATION_TYPE::AUTH};
    op2.add_TLV(TLV_TYPE::USRN, username.size(), username.c_str());
    op2.add_TLV(TLV_TYPE::PSWD, password.size(), password.c_str());
//    boost::array<char, 128> buffer;
//    buffer[0] = 'a';
//    buffer[1] = 'b';
    boost::system::error_code error;
    std::cout << op2.raw_op.size() << std::endl;
    std::string s {op2.raw_op.begin(), op2.raw_op.end()};
    std::cout << s << std::endl;
    socket.write_some(boost::asio::buffer(op2.raw_op), error);

//    buffer[0] = '5'; // AUTH
//    buffer[1] = '0'; // USR
//    int max_length = 4;
//    int pos = 2;
//
//    sprintf(buffer.data(), "50%04lu%s1%04lu%s",
//            username.length(),
//            username.c_str(),
//            password.length(),
//            password.c_str()
//            );
//    std::cout << buffer.data() << std::endl;
//    /*for (int i=0; i<max_length; i++) {
//        buffer[pos + i] = (length >> (max_length-1-i)*8)&0xFF;
//    }*/
//
//    boost::system::error_code error;
//    size_t len = socket.write_some(boost::asio::buffer(buffer), error);
//    if (error) {
//        std::cerr << "Failed to communicate with server." << std::endl;
//        return false;
//    }
    return true;

//            else if (error) throw boost::system::system_error(error);
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
            return -3;
        }

        boost::asio::io_context io_context;
        boost::asio::ip::tcp::resolver resolver{io_context};
        boost::asio::ip::tcp::resolver::results_type endpoints =
                resolver.resolve(hostname, portno); // equivalente a gethostbyname
        boost::asio::ip::tcp::socket socket{io_context};
        boost::asio::connect(socket, endpoints);
        authenticate(socket);

//        std::shared_ptr<LockedDirectory> ld = LockedDirectory::get_instance(path_to_watch);
//        std::shared_ptr<PendingOperationsQueue> poq = PendingOperationsQueue::get_instance();
//        FileWatcher fw{ld, poq, std::chrono::milliseconds(DELAY_MS)};
//        boost::thread fw_thread{[](FileWatcher &fw) {
//            fw.sync_watched_directory();
//            fw.start();
//        }, std::ref(fw)};
//        boost::thread op_thread{op_handler, std::ref(socket), std::ref(poq), std::cref(ld)};
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
        return -1;
    }
    catch (boost::system::system_error &e) {
        std::cerr << "System error with code " << e.code() << ": " << e.what() << std::endl;
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return -4;
    }
    return 0;
}