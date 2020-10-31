cmake_minimum_required(VERSION 3.17)
project(remote-backup-m1-client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS -pthread)
#[[set(Boost_USE_STATIC_LIBS ON)]]
#[[set(OPENSSL_USE_STATIC_LIBS TRUE)]]

set(BOOST_ROOT boost)
find_package(Boost 1.73.0 REQUIRED COMPONENTS filesystem regex thread)
find_package(OpenSSL REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
add_executable(remote-backup-m1-client main.cpp SSL_Connection.cpp SSL_Connection.h file_watcher.cpp file_watcher.h Directory.cpp Directory.h Metadata.cpp Metadata.h op.cpp op.h operation_queue.cpp operation_queue.h Parser.cpp Parser.h)

target_link_libraries(remote-backup-m1-client ${Boost_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)