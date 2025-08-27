add_library(grlang.node)
add_library(grlang::node ALIAS grlang.node)

target_compile_features(grlang.node PUBLIC cxx_std_23)

target_sources(grlang.node PRIVATE "src/node.cpp")
target_sources(
    grlang.node
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "include"
        FILES "include/grlang/node.h"
)

set_target_properties(
    grlang.node
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON EXPORT_NAME node
)

if(GRLANG_NODE_BUILD_TESTS)
    add_executable(grlang_node_test "test/node.test.cpp")
    target_link_libraries(grlang_node_test PRIVATE grlang.node grlang::node)
    add_test(NAME grlang_node_test COMMAND grlang_node_test)
endif()
