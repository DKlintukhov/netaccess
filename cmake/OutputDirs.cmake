# Normalized artifact layout.
# Executables, DLLs and PDBs land in <binaryDir>/bin and static/shared
# libraries in <binaryDir>/lib instead of being scattered across the CMake
# subdirectories.
# With cmake_layout() (Conan) the build dir is already per-configuration
# (e.g. build/Release), so no suffix is appended.
# Only multi-config generators (Visual Studio) get the $<CONFIG> subfolder.

if(DEFINED CMAKE_CONFIGURATION_TYPES)
    set(NETACCESS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
else()
    set(NETACCESS_OUTPUT_DIR "${CMAKE_BINARY_DIR}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${NETACCESS_OUTPUT_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${NETACCESS_OUTPUT_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${NETACCESS_OUTPUT_DIR}/lib")
set(CMAKE_PDB_OUTPUT_DIRECTORY "${NETACCESS_OUTPUT_DIR}/bin")
