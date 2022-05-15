cmake_minimum_required(VERSION 3.21)
project(OS_05)

set(CMAKE_CXX_STANDARD 20)

#add_executable(main  server.h)
add_executable(server main.cpp server.cpp server.h)
add_executable(client client.cpp)