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
