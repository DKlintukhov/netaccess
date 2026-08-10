# Sanitizers (ASan/UBSan). Usage: netaccess_enable_sanitizers(<target>)
# Controlled by the NETACCESS_ENABLE_SANITIZERS option (default ON).
# Applied only to Debug builds via a generator expression, so the check works
# with both single-config (Make/Ninja) and multi-config (Visual Studio)
# generators — CMAKE_BUILD_TYPE is empty for multi-config generators.

function(netaccess_enable_sanitizers target)
    if(NOT NETACCESS_ENABLE_SANITIZERS)
        return()
    endif()
    if(MSVC)
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:/fsanitize=address>
        )
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Debug>:/fsanitize=address>
        )
    else()
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-fsanitize=address,undefined>
            $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
        )
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Debug>:-fsanitize=address,undefined>
        )
    endif()
endfunction()
