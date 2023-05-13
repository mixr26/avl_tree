cmake_minimum_required(VERSION 3.5)

project(AVL_tree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic")

add_executable(AVL_tree main.cpp avl_tree.h)

install(TARGETS AVL_tree
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
