# netaccess_common: shared protocol/DTO library used by server and client.
# Drop your .cpp/.h files into this directory — they are picked up automatically.

file(GLOB_RECURSE NETACCESS_COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

set(NETACCESS_CONFIG_IN "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in")
configure_file("${NETACCESS_CONFIG_IN}" "${CMAKE_CURRENT_BINARY_DIR}/netaccess/config.h" @ONLY)

if(NETACCESS_COMMON_SOURCES)
    add_library(netaccess_common STATIC ${NETACCESS_COMMON_SOURCES})

    target_include_directories(netaccess_common PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
    )

    target_link_libraries(netaccess_common PUBLIC Qt6::Core)

    netaccess_enable_strict_warnings(netaccess_common)
    netaccess_enable_sanitizers(netaccess_common)
endif()
