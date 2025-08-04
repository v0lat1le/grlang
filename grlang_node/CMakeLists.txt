add_library(grlang_node src/node.cpp)
target_include_directories(grlang_node PUBLIC include)
target_link_libraries(grlang_node PUBLIC project_cxx_options)
