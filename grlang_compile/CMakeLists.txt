add_library(grlang.compile)
add_library(grlang::compile ALIAS grlang.compile)

target_compile_features(grlang.compile PUBLIC cxx_std_23)

target_sources(grlang.compile PRIVATE "src/compile.cpp")
target_sources(
    grlang.compile
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "include"
        FILES "include/grlang/compile.h"
)

set_target_properties(
    grlang.compile
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON EXPORT_NAME compile
)

target_link_libraries(grlang.compile PUBLIC grlang::node)

if(GRLANG_COMPILE_BUILD_TESTS)
    add_executable(grlang_compile_test "test/compile.test.cpp")
    target_link_libraries(grlang_compile_test PRIVATE grlang::compile grlang::parse grlang::node grtest)
    grtest_discover_tests(grlang_compile_test)
endif()
