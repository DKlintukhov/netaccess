# netaccess_server: server application (TCP listener + PostgreSQL).
# Drop your .cpp/.h files into this directory — they are picked up automatically.

file(GLOB_RECURSE NETACCESS_SERVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

if(NETACCESS_SERVER_SOURCES)
    add_executable(netaccess_server ${NETACCESS_SERVER_SOURCES})

    target_link_libraries(netaccess_server PRIVATE
        netaccess_common
        Qt6::Core
        Qt6::Network
        Qt6::Sql
    )

    netaccess_enable_strict_warnings(netaccess_server)
    netaccess_enable_sanitizers(netaccess_server)
    netaccess_deploy_qt_runtime(netaccess_server)
endif()
