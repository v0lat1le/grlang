function(grtest_discover_tests target)
    set(ctest_file_base "${CMAKE_CURRENT_BINARY_DIR}/${target}")
    set(ctest_include_file "${ctest_file_base}_include.cmake")
    set(ctest_tests_file "${ctest_file_base}_tests.cmake")
    add_custom_command(
        TARGET ${target} POST_BUILD
        BYPRODUCTS "${ctest_tests_file}"
        COMMAND ${target} list-tests ${target} "${ctest_tests_file}"
        VERBATIM
    )
    file(WRITE "${ctest_include_file}"
      "if(EXISTS \"${ctest_tests_file}\")\n"
      "  include(\"${ctest_tests_file}\")\n"
      "else()\n"
      "  add_test(${target}_NOT_BUILT ${target}_NOT_BUILT)\n"
      "endif()\n"
    )
    set_property(DIRECTORY
        APPEND PROPERTY TEST_INCLUDE_FILES "${ctest_include_file}"
    )
endfunction()
