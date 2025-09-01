add_library(grlang.eval)
add_library(grlang::eval ALIAS grlang.eval)

target_compile_features(grlang.eval PUBLIC cxx_std_23)

target_sources(
    grlang.eval
    PRIVATE
        "src/eval.cpp"
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "include"
        FILES "include/grlang/eval.h"
)

set_target_properties(
    grlang.eval
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON
)

target_link_libraries(grlang.eval PUBLIC grlang::node)

if(GRLANG_EVAL_BUILD_TESTS)
    add_executable(grlang_eval_test "test/eval.test.cpp")
    target_link_libraries(grlang_eval_test PRIVATE grlang::eval grlang::parse grlang::node grtest)
    grtest_discover_tests(grlang_eval_test)
endif()
