cmake_minimum_required (VERSION 3.12)

project(grlang VERSION 0.0.0 LANGUAGES CXX)


include(CTest)
set(ENABLE_TESTING True)


add_library(project_cxx_options INTERFACE)
target_compile_features(project_cxx_options INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(project_cxx_options INTERFACE /W4)
else()
    target_compile_options(project_cxx_options INTERFACE -Wall -Wextra -Wpedantic)
endif(MSVC)


add_subdirectory(grlang_node)
add_subdirectory(grlang_parse)
