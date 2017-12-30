cmake_minimum_required(VERSION 3.9)
project(Liso C)

set(CMAKE_C_STANDARD 99)

add_executable(echo_server liso.c clog.c clog.h parse.c parse.h)

add_executable(echo_client.c echo_client.c)

add_executable(parser example.c parse.h parse.c)

add_executable(cheer cheer.c)
