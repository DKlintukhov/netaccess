# Developer tooling targets: clang-tidy, clang-format check.

# Run clang-tidy as part of the build when NETACCESS_ENABLE_CLANG_TIDY is ON.
if(NETACCESS_ENABLE_CLANG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY
        clang-tidy
        -p=${CMAKE_BINARY_DIR}
        -header-filter=^${CMAKE_SOURCE_DIR}/
        -warnings-as-errors=*
    )
endif()

# clang-format --check across all project sources.
# The target is a no-op when clang-format is unavailable or there are no
# sources yet (clang-format with an empty file list would block on stdin).
find_program(NETACCESS_CLANG_FORMAT clang-format)
file(GLOB_RECURSE NETACCESS_FORMAT_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.hpp
    ${CMAKE_SOURCE_DIR}/src/*.cxx
    ${CMAKE_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_SOURCE_DIR}/tests/*.h
)
if(NETACCESS_CLANG_FORMAT AND NETACCESS_FORMAT_SOURCES)
    add_custom_target(clang-format-check
        COMMAND ${NETACCESS_CLANG_FORMAT} --dry-run --Werror ${NETACCESS_FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Checking code formatting with clang-format"
    )
else()
    add_custom_target(clang-format-check)
endif()
