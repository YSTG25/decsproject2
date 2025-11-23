cmake_minimum_required(VERSION 3.10)
project(kv_server)

set(CMAKE_CXX_STANDARD 17)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

include_directories(${LIBPQXX_INCLUDE_DIRS})
link_directories(${LIBPQXX_LIBRARY_DIRS})

add_executable(kv_server server/kv_server.cpp)
target_link_libraries(kv_server ${LIBPQXX_LIBRARIES} pthread)

add_executable(loadgen client/loadgen.cpp)
target_link_libraries(loadgen pthread)