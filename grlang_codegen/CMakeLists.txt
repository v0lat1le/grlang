add_library(grlang.codegen)
add_library(grlang::codegen ALIAS grlang.codegen)

target_compile_features(grlang.codegen PUBLIC cxx_std_23)

target_sources(grlang.codegen PRIVATE "src/codegen.cpp")
target_sources(
    grlang.codegen
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "include"
        FILES "include/grlang/codegen.h"
)

set_target_properties(
    grlang.codegen
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON
)

target_link_libraries(grlang.codegen PUBLIC grlang::node)

if(GRLANG_DEGEN_BUILD_TESTS)
    add_executable(grlang_codegen_test "test/codegen.test.cpp")
    target_link_libraries(grlang_codegen_test PRIVATE grlang::codegen grlang::parse grlang::node grtest)
    grtest_discover_tests(grlang_codegen_test)
endif()
