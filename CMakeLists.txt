cmake_minimum_required(VERSION 3.17)
project(remote-backup-m1-client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS -pthread)
#[[set(Boost_USE_STATIC_LIBS ON)]]
#[[set(OPENSSL_USE_STATIC_LIBS TRUE)]]

set(BOOST_ROOT boost)
find_package(Boost 1.73.0 REQUIRED COMPONENTS filesystem regex thread serialization program_options)
find_package(OpenSSL REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
add_executable(remote-backup-m1-client main.cpp core/file_watcher.cpp core/file_watcher.h communication/message.cpp communication/message.h communication/tlv_view.cpp communication/tlv_view.h utilities/tools.cpp utilities/tools.h core/connection.cpp core/connection.h directory/dir.cpp directory/dir.h directory/resource.cpp directory/resource.h core/scheduler.cpp core/scheduler.h communication/f_message.cpp communication/f_message.h core/user.cpp core/user.h communication/types.h)

target_link_libraries(remote-backup-m1-client ${Boost_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)