#include <iostream>
#include <boost/filesystem.hpp>
#include "FileWatcher.h"

using namespace boost::filesystem;

void f(PendingOperationsQueue &poq, std::shared_ptr<WatchedDirectory> const &wd_ptr) {
//    while (1) {
    Operation op = poq.next_op();
    switch (op.get_type()) {
        case OPERATION_TYPE::CREATE:
            //...contatta il server
            wd_ptr->insert(op.get_path());
            break;
        case OPERATION_TYPE::DELETE:
            //...contatta il server
            wd_ptr->erase(op.get_path());
            break;
        case OPERATION_TYPE::UPDATE:
            //...contatta il server
            wd_ptr->refresh_hash(op.get_path());
            break;
        default:
            break;
    }
//    }
}


int main(int argc, char *argv[]) {
    if (argc < 2) return -1;
    // check input folder existance
    // check
    std::shared_ptr<WatchedDirectory> wd_ptr = WatchedDirectory::get_instance(argv[1]);
    FileWatcher fw{wd_ptr, std::chrono::seconds(5)};
    PendingOperationsQueue poq{};
    fw.start(poq);
    std::thread t (f, std::ref(poq), std::cref(wd_ptr));
    return 0;
}