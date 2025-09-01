add_library(grlang.parse)
add_library(grlang::parse ALIAS grlang.parse)

target_compile_features(grlang.parse PUBLIC cxx_std_23)

target_sources(
    grlang.parse
    PRIVATE
        "src/parse.cpp"
        "src/token.cpp"
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "include"
        FILES "include/grlang/parse.h"
)

set_target_properties(
    grlang.parse
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON
)

target_link_libraries(grlang.parse PUBLIC grlang::node)

if(GRLANG_PARSE_BUILD_TESTS)
    add_executable(grlang_parse_test "test/parse.test.cpp")
    target_link_libraries(grlang_parse_test PRIVATE grlang::parse grlang::node grtest)
    grtest_discover_tests(grlang_parse_test)
endif()
