# Actual production operation/adapter bodies, compiled with the configured toolchain.
include("${CMAKE_CURRENT_SOURCE_DIR}/../manual_test_warnings.cmake")
set(reset_generator "${CMAKE_CURRENT_SOURCE_DIR}/check.py")
get_filename_component(reset_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
foreach(variant current persist initial_load expiry owner restore_mode cleanup_scope)
    set(reset_output "${CMAKE_CURRENT_BINARY_DIR}/${variant}.cpp")
    set(reset_options)
    if(NOT variant STREQUAL current)
        list(APPEND reset_options --rollback ${variant})
    endif()
    add_custom_command(OUTPUT "${reset_output}"
        COMMAND "${Python3_EXECUTABLE}" -B "${reset_generator}" ${reset_options} --emit "${reset_output}"
        DEPENDS "${reset_generator}" "${reset_root}/Extension/ArcadeMode/arcade_mode.cpp"
            "${reset_root}/Runtime/console_bridge.cpp" "${reset_root}/Runtime/client_tick.cpp"
            "${reset_root}/Extension/Console/commands.cpp"
        VERBATIM)
    if(variant STREQUAL current)
        set(target dingosdk_arcade_runtime_reset_tests)
    else()
        set(target dingosdk_arcade_runtime_reset_${variant}_control)
    endif()
    add_executable(${target} "${reset_output}")
    target_compile_features(${target} PRIVATE cxx_std_20)
    dingosdk_arcade_manual_test_warnings(${target})
    if(variant STREQUAL current)
        add_test(NAME arcade_runtime_reset COMMAND ${target})
    else()
        add_dependencies(dingosdk_arcade_runtime_reset_tests ${target})
        add_test(NAME arcade_runtime_reset_rollback_${variant}
            COMMAND "${Python3_EXECUTABLE}" -B "${reset_generator}" --rollback ${variant}
                --expect-binary $<TARGET_FILE:${target}>)
    endif()
endforeach()
