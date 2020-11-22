#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/regex.hpp>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>
#include <netinet/in.h>
#include "file_watcher.h"
#include "message.h"
#include "tlv_view.h"
#include "tools.h"
#include "connection.h"

#define WRITE_MASK 4 << 6
#define DELAY_MS 5000


bool f(communication::tlv_view const &c_view, communication::tlv_view const &s_view,
       std::function<bool(std::vector<std::string> const &)> const &exec) {
    if (s_view.get_tlv_type() == communication::TLV_TYPE::ITEM) {
        std::string c_sign{c_view.cbegin(), c_view.cend()};
        std::string s_sign{s_view.cbegin(), s_view.cend()};
        if (s_sign != c_sign) {
            std::cerr << "Server manages a different signature"
                      << "\nC:" << c_sign
                      << "\nS:" << s_sign << std::endl;
            return false;
        } else return exec(tools::split_sign(s_sign));
    } else {
        std::cerr << "Wrong server response format" << std::endl;
        return false;
    }
}

//void msg_handler(connection &connection, std::shared_ptr<communication::message_queue> &poq,
//                std::shared_ptr<directory::dir> const &c_dir) {
//    while (true) {
//        communication::message client_msg = poq->pop();
//        connection.write(client_msg);
//        communication::message server_msg = connection.read();
//        // the server and the client communication types are the same
//        communication::MESSAGE_TYPE c_msg_type = client_msg.get_msg_type();
//        communication::MESSAGE_TYPE s_msg_type = server_msg.get_msg_type();
//        communication::tlv_view c_view{client_msg};
//        // reading response from the server...
//        communication::tlv_view s_view{server_msg};
//
//        if (s_msg_type != c_msg_type) {
//            std::cerr << "Wrong server response" << std::endl;
//            std::exit(-1);
//        }
//        if (!s_view.next_tlv()) {
//            std::cerr << "Failed to read server message" << std::endl;
//            std::exit(-1);
//        }
//        auto s_tlv_type = s_view.get_tlv_type();
//        if (s_tlv_type == communication::TLV_TYPE::ERROR) {
//            std::cerr << "Operation failed: " << std::string{s_view.begin(), s_view.end()} << std::endl;
//            std::exit(-2);
//        }
//
//        c_view.next_tlv();
//        if (c_msg_type == communication::MESSAGE_TYPE::CREATE) {
//            if (!f(c_view, s_view, [&c_dir](auto results) {
//                return c_dir->insert(results[1], results[2]);
//            })) {
//                std::exit(-1);
//            }
//        } else if (s_msg_type == communication::MESSAGE_TYPE::UPDATE) {
//            if (!f(c_view, s_view, [&c_dir](auto results) {
//                return c_dir->update(results[1], results[2]);
//            })) {
//                std::exit(-1);
//            }
//        } else if (s_msg_type == communication::MESSAGE_TYPE::DELETE) {
//            if (!f(c_view, s_view, [&c_dir](auto results) {
//                return c_dir->erase(results[1]);
//            })) {
//                std::exit(-1);
//            }
//        } else if (s_msg_type == communication::MESSAGE_TYPE::SYNC) {
//            auto s_dir = directory::dir::get_instance("S_DIR");
//            do {
//                if (s_view.get_tlv_type() == communication::TLV_TYPE::ITEM) {
//                    std::string s_sign{s_view.cbegin(), s_view.cend()};
//                    auto sign_parts = tools::split_sign(s_sign);
//                    std::pair<bool, bool> pair = c_dir->contains_and_match(sign_parts[0], sign_parts[1]);
//                    // if doesn't exist
//                    if (!pair.first) {
//                        auto msg = communication::message{communication::MESSAGE_TYPE::DELETE};
//                        msg.add_TLV(communication::TLV_TYPE::ITEM, s_sign.size(), s_sign.c_str());
//                        msg.add_TLV(communication::TLV_TYPE::END);
//                        poq->push(msg);
//                        // if hash doesn't match
//                    } else if (!pair.second) {
//                        auto msg = communication::message{communication::MESSAGE_TYPE::UPDATE};
//                        msg.add_TLV(communication::TLV_TYPE::ITEM, s_sign.size(), s_sign.c_str());
//                        msg.add_TLV(communication::TLV_TYPE::END);
//                        poq->push(msg);
//                    }
//                    if (!s_dir->insert(sign_parts[0], sign_parts[1])) {
//                        std::cerr << "Failed to store server item" << std::endl;
//                        std::exit(-1);
//                    }
//                }
//            } while (s_view.next_tlv());
//
//            c_dir->for_each_if([&s_dir](boost::filesystem::path const &path) {
//                return !s_dir->contains(path);
//            }, [&poq](std::pair<boost::filesystem::path, std::string> const &pair) {
//                auto msg = communication::message{communication::MESSAGE_TYPE::CREATE};
//                msg.add_TLV(communication::TLV_TYPE::ITEM, pair.second.size(), pair.second.c_str());
//                msg.add_TLV(communication::TLV_TYPE::FILE, pair.first);
//                msg.add_TLV(communication::TLV_TYPE::END);
//                poq->push(msg);
//            });
//
//        } else {
//            std::cerr << "Communication failed" << std::endl;
//            std::exit(-1);
//        }
//    }
//}


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

        if (!conn->authenticate()) {
            std::cerr << "Authentication failed" << std::endl;
            return -3;
        }
        conn->authenticate();
        scheduler->schedule_sync();



//        auto poq = communication::message_queue::get_instance();
//        auto dir = directory::dir::get_instance(path_to_watch, true);

        //

//        poq->push(sync_op);

        file_watcher fw{dir2, conn, std::chrono::milliseconds{DELAY_MS}};
//        boost::thread op_thread{msg_handler, std::ref(conn), std::ref(poq), std::cref(dir)};
        boost::thread fw_thread{[](file_watcher &fw) {
            fw.start();
        }, std::ref(fw)};
        fw_thread.join();
//        op_thread.join();
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