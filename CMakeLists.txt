cmake_minimum_required (VERSION 3.31)

project(grlang VERSION 0.1.0 LANGUAGES CXX)

enable_testing()

if(GRLANG_NODE_BUILD_TESTS OR GRLANG_PARSE_BUILD_TESTS OR GRLANG_COMPILE_BUILD_TESTS OR GRLANG_EVAL_BUILD_TESTS)
add_subdirectory(grtest)
endif()
add_subdirectory(grlang_node)
add_subdirectory(grlang_parse)
add_subdirectory(grlang_compile)
if(GRLANG_EVAL_BUILD OR GRLANG_EVAL_BUILD_TESTS)
    add_subdirectory(grlang_eval)
endif()
