# Deploys the runtime next to a Qt executable on Windows.
#
# Qt side: windeployqt copies the Qt DLLs, platform plugins, QML imports and
# runtime dependencies into the executable's output directory (<build>/<config>/bin)
# after every build, so the exe can be run standalone without activating the Conan
# environment. windeployqt is located through CMAKE_PREFIX_PATH.
#
# MSVC side: windeployqt only fetches the MSVC runtime (Debug CRT included) when
# VCINSTALLDIR points at the Visual C++ installation, so we set it from the
# compiler location. The AddressSanitizer runtime (clang_rt.asan_dynamic-x86_64.dll,
# Debug + ASan only) is not known to windeployqt and is copied explicitly.
#
# For distribution/install the Qt-recommended mechanism is instead
# qt_generate_deploy_app_script() -- add it later when packaging is introduced.

function(netaccess_deploy_qt_runtime target)
    if(NOT WIN32)
        return()
    endif()

    cmake_parse_arguments(ARG "" "QML_DIR" "" ${ARGN})

    # --- Qt runtime (windeployqt) --------------------------------------------------
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt6
        PATHS ${CMAKE_PREFIX_PATH}
        NO_DEFAULT_PATH
    )

    if(WINDEPLOYQT_EXECUTABLE)
        set(_deploy_args "")

        if(ARG_QML_DIR)
            if(IS_DIRECTORY "${ARG_QML_DIR}")
                list(APPEND _deploy_args "--qmldir" "${ARG_QML_DIR}")
            else()
                message(WARNING "QML dir '${ARG_QML_DIR}' not found; windeployqt will not scan QML imports")
            endif()
        endif()

        set(_windeploy_command "${WINDEPLOYQT_EXECUTABLE}")
        if(MSVC AND CMAKE_CXX_COMPILER)
            # Walk up from cl.exe to the VC root -- the directory that contains
            # redist/MSVC (e.g. .../BuildTools/VC). EXISTS is case-insensitive
            # on Windows, so this matches "Redist/MSVC" on disk.
            set(_vc_root "")
            set(_p "${CMAKE_CXX_COMPILER}")
            set(_depth 0)
            while(NOT _vc_root AND _depth LESS 12)
                get_filename_component(_p "${_p}" DIRECTORY)
                if(EXISTS "${_p}/redist/MSVC")
                    set(_vc_root "${_p}")
                endif()
                math(EXPR _depth "${_depth} + 1")
            endwhile()
            if(_vc_root)
                set(_windeploy_command
                    "${CMAKE_COMMAND}" -E env "VCINSTALLDIR=${_vc_root}"
                    "${WINDEPLOYQT_EXECUTABLE}")
            endif()
        endif()

        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${_windeploy_command}
                    "$<TARGET_FILE:${target}>"
                    "--no-translations"
                    ${_deploy_args}
            COMMENT "Deploying Qt runtime for ${target}"
            VERBATIM
        )
    else()
        message(STATUS "windeployqt not found; skipping Qt runtime deployment for ${target}")
    endif()

    # --- MSVC AddressSanitizer runtime (Debug + ASan only) ---------------------------
    if(MSVC AND CMAKE_CXX_COMPILER)
        get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(_asan "${_compiler_dir}/clang_rt.asan_dynamic-x86_64.dll")
        if(EXISTS "${_asan}")
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND "${CMAKE_COMMAND}"
                        -E copy_if_different "${_asan}" "$<TARGET_FILE_DIR:${target}>"
                COMMENT "Deploying MSVC ASan runtime for ${target}"
                VERBATIM
            )
        endif()
    endif()
endfunction()
