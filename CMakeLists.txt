cmake_minimum_required(VERSION 3.10)
project(kv_server)

set(CMAKE_CXX_STANDARD 17)

add_executable(kv_server server/kv_server.cpp)
add_executable(loadgen client/loadgen.cpp)
