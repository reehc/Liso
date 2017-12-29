cmake_minimum_required(VERSION 3.9)
project(Liso C)

set(CMAKE_C_STANDARD 99)

add_executable(echo_server echo_server.c)

add_executable(echo_client.c echo_client.c)

add_executable(select select.c)
