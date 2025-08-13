cmake_minimum_required (VERSION 3.31)

project(grlang VERSION 0.1.0 LANGUAGES CXX)

enable_testing()

add_subdirectory(grlang_node)
add_subdirectory(grlang_parse)
