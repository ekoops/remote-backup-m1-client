#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <boost/bind/bind.hpp>
#include "file_watcher.h"
#include "message.h"
#include "connection.h"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

void terminate(
        boost::asio::io_context &io,
        file_watcher &fw,
        std::shared_ptr<scheduler> const &scheduler_ptr,
        std::shared_ptr<connection> const &connection_ptr
) {
    std::cout << "Terminating..." << std::endl;
//    io.stop();
//    fw.stop();
//    scheduler_ptr->close();
//    connection_ptr->close();
    std::cout << "SCIACCA" << std::endl;
}


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
        if (scheduler->auth(username, password)) return true;
        else std::cout << "Authentication failed (attempts left " << --general_attempts << ")." << std::endl;
    }
    return false;
}

po::variables_map parse_options(int argc, char const *const argv[]) {
    try {
        po::options_description desc("Backup client options");
        desc.add_options()
                ("help,h",
                 "produce help message")
                ("path-to-watch,P",
                 po::value<fs::path>()->default_value(
                         fs::path{"."}
                 ), "set path to watch")
                ("hostname,H",
                 po::value<std::string>()->required(),
                 "set backup server hostname")
                ("service,S",
                 po::value<std::string>()->required(),
                 "set backup server service name/port number")
                ("threads,T",
                 po::value<size_t>()->default_value(8),
                 "set worker thread pool size")
                ("delay,D",
                 po::value<size_t>()->default_value(5000),
                 "set file watcher refresh rate in milliseconds");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            std::exit(EXIT_SUCCESS);
        }

        po::notify(vm);

        // canonical returns the absolute path resolving symbolic link, dot and dot-dot
        // it also checks the existence
        vm.at("path-to-watch").value() = fs::canonical(vm["path-to-watch"].as<fs::path>());
        auto path_to_watch = vm["path-to-watch"];
        if (path_to_watch.defaulted()) {
            std::cout << "--path-to-watch option set to default value: "
                      << path_to_watch.as<fs::path>() << std::endl;
        } else {
            auto dir = path_to_watch.as<fs::path>();
            if (!fs::is_directory(dir)) {
                std::cerr << dir << " is not a directory" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
        if (vm["threads"].defaulted()) {
            std::cout << "--threads option set to default value: "
                      << vm["threads"].as<size_t>() << std::endl;
        }
        if (vm["delay"].defaulted()) {
            std::cout << "--delay option set to default value: "
                      << vm["delay"].as<size_t>() << std::endl;
        }
        return vm;
    }
    catch (std::exception &ex) {
        std::cout << "Error during options parsing: " << ex.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *const argv[]) {
    // Parsing program options
    po::variables_map vm = parse_options(argc, argv);

    try {
        // Destructuring program options
        fs::path path_to_watch = vm["path-to-watch"].as<fs::path>();
        std::string hostname = vm["hostname"].as<std::string>();
        std::string service = vm["service"].as<std::string>();
        size_t thread_pool_size = vm["threads"].as<size_t>();
        size_t delay = vm["delay"].as<size_t>();

        // Constructing an abstraction for the watched directory
        auto watched_dir_ptr = directory::dir::get_instance(path_to_watch, true);

        boost::asio::io_context io_context;
        // Allowing generic SSL/TLS version
        boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23};
        ctx.load_verify_file("../certs/ca.pem");
        // Constructing an abstraction for handling SSL connection task
        auto connection_ptr = connection::get_instance(io_context, ctx, thread_pool_size);
        // Constructing an abstraction for scheduling task and managing communication
        // with server through the connection
        auto scheduler_ptr = scheduler::get_instance(io_context, watched_dir_ptr, connection_ptr, thread_pool_size);
        // Constructing an abstraction for monitoring the filesystem and scheduling
        // server synchronizations through scheduler
        file_watcher fw{watched_dir_ptr, scheduler_ptr, std::chrono::milliseconds{delay}};

//        // Setting callback to handle process signals
//        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
//        // Start an asynchronous wait for one of the signals to occur.
//        signals.async_wait(boost::bind(
//                &terminate,
//                boost::ref(io_context),
//                boost::ref(fw),
//                boost::cref(scheduler_ptr),
//                boost::cref(connection_ptr)
//        ));
        // Performing server connection
        connection_ptr->connect(hostname, service);
        // Starting login procedure
        if (!login(scheduler_ptr)) {
            std::cerr << "Authentication failed" << std::endl;
            return EXIT_FAILURE;
        }
        // Starting specified directory local file watching
        fw.start();
    }
    catch (fs::filesystem_error &e) {
        std::cerr << "Filesystem error from " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (boost::system::system_error &e) {
        std::cerr << "System error with code " << e.code() << ": " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}