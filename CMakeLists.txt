cmake_minimum_required(VERSION 3.17)
project(socket_perf)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -Ofast")

add_executable(socket_echo_server server/socket_echo_server.cpp)
add_executable(socket_echo_client client/socket_echo_client.cpp client/socket_processor.cpp)

TARGET_LINK_LIBRARIES(socket_echo_client pthread)