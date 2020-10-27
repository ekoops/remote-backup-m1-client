#include <iostream>
#include <boost/filesystem.hpp>
#include <csignal>
#include <netinet/in.h>
#include <netdb.h>
#include "FileWatcher.h"
#include "LockedDirectory.h"
#include "Socket.h"

#define READ_AND_WRITE_MASK 6 << 6
#define DELAY_MS 5000

using namespace boost::filesystem;

void SIGINT_handler(int signum) {
    std::cout << "SIGINT" << std::endl;
}

void SIGQUIT_handler(int signum) {
    exit(signum);
}

void operations_handler(Socket& socket, std::shared_ptr<PendingOperationsQueue>& poq,
              std::shared_ptr<LockedDirectory> const &ld) {
    while (true) {
        std::shared_ptr<Operation> op_ptr = poq->pop();
        std::ostringstream oss;
        std::cout << oss.str();

        switch (op_ptr->get_type()) {
            case OPERATION_TYPE::CREATE:
                //...contatta il server
                oss.clear();
                oss << "CREATE operation on " << op_ptr->get_path() << std::endl;
                std::cout << oss.str();
                ld->insert(op_ptr->get_path());
                break;
            case OPERATION_TYPE::DELETE:
                //...contatta il server
                oss.clear();
                oss << "DELETE operation on " << op_ptr->get_path() << std::endl;
                std::cout << oss.str();
                ld->erase(op_ptr->get_path());
                break;
            case OPERATION_TYPE::UPDATE:
                //...contatta il server
                oss.clear();
                oss << "UPDATE operation on " << op_ptr->get_path() << std::endl;
                std::cout << oss.str();
                ld->update(op_ptr->get_path(), Metadata {op_ptr->get_path()});
                break;
            default:
                break;
        }
    }
}

void watch_handler(FileWatcher& fw, std::shared_ptr<PendingOperationsQueue>& poq) {
    fw.sync_watched_directory();
    fw.start();
}

int main(int argc, char const * const argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: ./name path_to_watch hostname portno" << std::endl;
        return -1;
    }
    path path_to_watch {argv[1]};
    std::string hostname {argv[2]};
    uint16_t portno;
    // Trying to parse port number
    try {
        portno = static_cast<uint16_t>(std::stoi(argv[3]));
    }
    catch (std::exception& ex) {
        std::cerr << "Failed to parse portno" << std::endl;
        return -2;
    }

    // Checking path existance and permissions...
    if (!exists(path_to_watch) ||
        !is_directory(path_to_watch) ||
        (status(path_to_watch).permissions() & READ_AND_WRITE_MASK) != READ_AND_WRITE_MASK) {
        std::cerr << "Path provided doesn't exist is not a directory or permission denied" << std::endl;
        return -3;
    }

    Socket socket {};
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Trying to resolve hostname...
    if (!(server = gethostbyname(hostname.c_str()))) {
        std::cerr << "Failed to resolve hostname " << hostname << std::endl;
        return -4;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // Trying to connect to the server...
    try {
        socket.connect(&serv_addr, sizeof(serv_addr));
    }
    catch (std::runtime_error& ex) {
        std::cerr << ex.what() << std::endl;
        return -5;
    }

    std::shared_ptr<LockedDirectory> ld = LockedDirectory::get_instance(path_to_watch);
    std::shared_ptr<PendingOperationsQueue> poq = PendingOperationsQueue::get_instance();
    //TODO condificare operazioni e definire protocollo di comunicazione
    poq->push(Operation::get_instance(OPERATION_TYPE::SYNC, path {"TODO"}));
    //TODO gestire le operazioni di sincronizzazione (sync_watched_directory) su operations_handler
    FileWatcher fw{ld, poq, std::chrono::milliseconds(DELAY_MS)};
    std::thread fw_thread {watch_handler, std::ref(fw), std::ref(poq)};
    std::thread op_thread {operations_handler, std::ref(socket), std::ref(poq), std::cref(ld)};
//    signal(SIGINT, SIGINT_handler);
//    signal(SIGQUIT, SIGQUIT_handler);
    fw_thread.join();
    op_thread.join();
    return 0;
}