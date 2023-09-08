cmake_minimum_required(VERSION 3.0)
project(telnetd C)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

find_package(unity QUIET)

add_subdirectory(src)
