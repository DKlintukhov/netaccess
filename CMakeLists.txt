cmake_minimum_required(VERSION 3.31)

project(netaccess
    VERSION 0.1.0
    DESCRIPTION "Client-server application for managing access to network resources"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NETACCESS_ENABLE_CLANG_TIDY "Run clang-tidy during the build" OFF)
option(NETACCESS_ENABLE_SANITIZERS "Enable address/undefined behavior sanitizers in Debug builds" ON)
option(NETACCESS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(NETACCESS_BUILD_TESTS "Build the unit tests" ON)

include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/OutputDirs.cmake)
include(cmake/Tooling.cmake)
include(cmake/QtDeploy.cmake)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    Network
    Sql
)

# Modern Qt 6 housekeeping: sets up AUTOMOC/AUTOUIC and enables the Qt CMake
# policies for the whole project. qt_standard_project_setup() does NOT enable
# AUTORCC, so turn it on here to compile the .qrc resource files.
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

enable_testing()

add_subdirectory(src/common)
add_subdirectory(src/server)
add_subdirectory(src/client)
if(NETACCESS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
