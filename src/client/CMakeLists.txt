# netaccess_client: Qt Quick (QML) client application.
# Drop your .cpp/.h/.qml/.qrc files into this directory — they are picked up automatically.

file(GLOB_RECURSE NETACCESS_CLIENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.qrc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ui"
)

if(NETACCESS_CLIENT_SOURCES)
    add_executable(netaccess_client ${NETACCESS_CLIENT_SOURCES})

    target_include_directories(netaccess_client PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
    )

    target_link_libraries(netaccess_client PRIVATE
        netaccess_common
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Network
    )

    netaccess_enable_strict_warnings(netaccess_client)
    netaccess_enable_sanitizers(netaccess_client)
    netaccess_deploy_qt_runtime(netaccess_client QML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qml")
endif()
