# Strict compiler warnings. Usage: netaccess_enable_strict_warnings(<target>)
# Warnings-as-errors uses the CMake 3.24+ COMPILE_WARNING_AS_ERROR target
# property. MSVC and GCC/Clang use different flag syntax, hence the branch;
# the GCC/Clang flags are supported by both compilers across the platforms we
# target (Linux, macOS, FreeBSD).

function(netaccess_enable_strict_warnings target)
    if(NETACCESS_WARNINGS_AS_ERRORS)
        set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
    endif()
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /sdl
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wformat=2
            -Wundef
        )
    endif()
endfunction()
