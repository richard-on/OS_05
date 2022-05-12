cmake_minimum_required(VERSION 3.22)
project(OS_05)

set(CMAKE_CXX_STANDARD 20)

add_executable(server server.cpp)
add_executable(client client.cpp)