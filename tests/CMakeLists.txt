# netaccess_tests: Qt Test unit tests.
# Drop your test .cpp files into this directory — they are picked up automatically.

find_package(Qt6 6.8 REQUIRED COMPONENTS Test)

file(GLOB_RECURSE NETACCESS_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

if(NETACCESS_TEST_SOURCES)
    add_executable(netaccess_tests ${NETACCESS_TEST_SOURCES})

    target_link_libraries(netaccess_tests PRIVATE
        netaccess_common
        Qt6::Core
        Qt6::Test
    )

    netaccess_enable_strict_warnings(netaccess_tests)
    netaccess_enable_sanitizers(netaccess_tests)

    add_test(NAME netaccess_tests COMMAND netaccess_tests)
endif()
