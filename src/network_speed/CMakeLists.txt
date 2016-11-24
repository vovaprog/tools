cmake_minimum_required(VERSION 2.8)

project (network_speed)

set(CMAKE_VERBOSE_MAKEFILE on)

include_directories("." "../cpp")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -pthread")

add_executable(server ./server.cpp ../cpp/NetworkUtils.cpp)
add_executable(client ./client.cpp ../cpp/NetworkUtils.cpp)

add_executable(server_read ./server_read.cpp ../cpp/NetworkUtils.cpp)
add_executable(client_write ./client_write.cpp ../cpp/NetworkUtils.cpp)

add_executable(server_poll ./server_poll.cpp ../cpp/NetworkUtils.cpp)
add_executable(server_epoll ./server_epoll.cpp ../cpp/NetworkUtils.cpp)

add_executable(server_read_unix ./server_read_unix.cpp ../cpp/NetworkUtils.cpp)
add_executable(client_write_unix ./client_write_unix.cpp ../cpp/NetworkUtils.cpp)

add_executable(fifo_speed ./fifo_speed.cpp)


