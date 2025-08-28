add_library(grtest INTERFACE)

target_sources(
    grtest
    INTERFACE
        FILE_SET HEADERS
        BASE_DIRS "include"
        FILES "include/grtest.h"
)

set_target_properties(
    grtest
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON
)

include("cmake/GrTest.cmake")
