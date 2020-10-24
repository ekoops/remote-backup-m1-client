#include <iostream>
#include <boost/filesystem.hpp>
#include <csignal>
#include "FileWatcher.h"
#include "LockedDirectory.h"

#define READ_AND_WRITE_MASK 6 << 6

using namespace boost::filesystem;

void SIGINT_handler(int signum) {
    std::cout << "SIGINT" << std::endl;
}

void SIGQUIT_handler(int signum) {
    exit(signum);
}

void operations_handler(PendingOperationsQueue &poq,
              std::shared_ptr<LockedDirectory> const &ld) {
    while (true) {
        Operation op = poq.next_op();
        std::ostringstream oss;
        std::cout << oss.str();

        switch (op.get_type()) {
            case OPERATION_TYPE::CREATE:
                //...contatta il server
                oss.clear();
                oss << "CREATE operation on " << op.get_path() << std::endl;
                std::cout << oss.str();
                ld->insert(op.get_path());
                break;
            case OPERATION_TYPE::DELETE:
                //...contatta il server
                oss.clear();
                oss << "DELETE operation on " << op.get_path() << std::endl;
                std::cout << oss.str();
                ld->erase(op.get_path());
                break;
            case OPERATION_TYPE::UPDATE:
                //...contatta il server
                oss.clear();
                oss << "UPDATE operation on " << op.get_path() << std::endl;
                std::cout << oss.str();
                ld->update(op.get_path(), Metadata {op.get_path()});
                break;
            default:
                break;
        }
    }
}

void watch_handler(FileWatcher& fw, PendingOperationsQueue& poq) {
    fw.start(poq);
}

int main(int argc, char const * const argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./name path_to_watch" << std::endl;
        return -1;
    }
    path path_to_watch{argv[1]};

    if (!exists(path_to_watch) ||
        !is_directory(path_to_watch) ||
        (status(path_to_watch).permissions() & READ_AND_WRITE_MASK) != READ_AND_WRITE_MASK) {
        std::cerr << "Path provided doesn't exist is not a directory or permission denied" << std::endl;
        return -2;
    }

    std::shared_ptr<LockedDirectory> ld = LockedDirectory::get_instance(path_to_watch);
    FileWatcher fw{ld, std::chrono::milliseconds(3000)};
    PendingOperationsQueue poq{};
    std::thread t(operations_handler, std::ref(poq), std::cref(ld));
    std::thread fw_thread {watch_handler, std::ref(fw), std::ref(poq)};
//    signal(SIGINT, SIGINT_handler);
//    signal(SIGQUIT, SIGQUIT_handler);
    fw_thread.join();
    t.join();
    return 0;
}