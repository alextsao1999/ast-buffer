# Created By Alex Tsao
cmake_minimum_required(VERSION 3.14)
project(ast-buffer)
set(CMAKE_CXX_STANDARD 14)

add_library(ast-buffer STATIC ./src/lib.c)
target_include_directories(ast-buffer PUBLIC ./include)

add_library(tree-sitter-cpp STATIC ./tree-sitter-cpp/src/parser.c ./tree-sitter-cpp/src/scanner.cc)
target_link_directories(tree-sitter-cpp PRIVATE ./tree-sitter-cpp/src)

add_executable(sitter-test test/main.cpp)
target_link_libraries(sitter-test tree-sitter-cpp ast-buffer)
