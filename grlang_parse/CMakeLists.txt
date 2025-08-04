add_library(grlang_parse src/parse.cpp)
target_include_directories(grlang_parse PUBLIC include)
target_link_libraries(grlang_parse PUBLIC project_cxx_options grlang_node)

if(ENABLE_TESTING)
    add_executable(grlang_parse_test test/parse_test.cpp)
    target_link_libraries(grlang_parse_test PRIVATE project_cxx_options grlang_parse)
    add_test(NAME grlang_parse_test COMMAND grlang_parse_test)
endif()
