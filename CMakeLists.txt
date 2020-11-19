cmake_minimum_required(VERSION 3.17)
project(remote-backup-m1-client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS -pthread)
#[[set(Boost_USE_STATIC_LIBS ON)]]
#[[set(OPENSSL_USE_STATIC_LIBS TRUE)]]

set(BOOST_ROOT boost)
find_package(Boost 1.73.0 REQUIRED COMPONENTS filesystem regex thread serialization)
find_package(OpenSSL REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
add_executable(remote-backup-m1-client main.cpp file_watcher.cpp file_watcher.h dir.cpp dir.h message.cpp message.h message_queue.cpp message_queue.h tlv_view.cpp tlv_view.h tools.cpp tools.h connection.cpp connection.h)

target_link_libraries(remote-backup-m1-client ${Boost_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)