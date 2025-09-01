add_library(grlang.codegen)
add_library(grlang::codegen ALIAS grlang.codegen)

target_compile_features(grlang.codegen PUBLIC cxx_std_23)

target_sources(
    grlang.codegen
    PRIVATE
        "src/codegen.cpp"
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

if(GRLANG_CODEGEN_LLVM_IR_BUILD_TESTS)
    add_executable(grlang_codegen_test "test/codegen.test.cpp")
    target_link_libraries(grlang_codegen_test PRIVATE grlang::codegen grlang::parse grlang::node)

    find_program(GRLANG_CLANG "clang")
    function(grl_codegen_test test_file test_input test_output)
        add_test(NAME "grlang_codegen_test.gen_${test_file}" COMMAND grlang_codegen_test "${CMAKE_CURRENT_LIST_DIR}/test/${test_file}.grl" "-o" "${test_file}.ll")
        add_test(NAME "grlang_codegen_test.cc_${test_file}"  COMMAND ${GRLANG_CLANG} "${test_file}.ll" "-o" "${test_file}${CMAKE_EXECUTABLE_SUFFIX}")
        add_test(NAME "grlang_codegen_test.run_${test_file}" COMMAND "${test_file}${CMAKE_EXECUTABLE_SUFFIX}" "${test_input}" "${test_output}")
        set_tests_properties("grlang_codegen_test.cc_${test_file}"  PROPERTIES DEPENDS "grlang_codegen_test.gen_${test_file}")
        set_tests_properties("grlang_codegen_test.run_${test_file}" PROPERTIES DEPENDS "grlang_codegen_test.cc_${test_file}")
    endfunction()

    grl_codegen_test(fib_loop 10 10)
    grl_codegen_test(fib_recurse 10 10)
endif()
