cmake_minimum_required(VERSION 3.24)
# Also usable without configuring the runtime or fetching its dependencies:
# cmake -S Extension/ArcadeMode/Test/manual_lifecycle -B <build-directory>
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ArcadeManualLifecycleTests LANGUAGES CXX)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()
get_filename_component(lifecycle_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(lifecycle_output "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(lifecycle_generator "${CMAKE_CURRENT_SOURCE_DIR}/generate.py")
set(lifecycle_variants current model_reset producer_revoke buffered_release bail)
set(lifecycle_sources)
foreach(variant IN LISTS lifecycle_variants)
    list(APPEND lifecycle_sources "${lifecycle_output}/${variant}.cpp")
endforeach()
set(lifecycle_outputs ${lifecycle_sources}
    "${lifecycle_output}/acquisition_without_bail.h" "${lifecycle_output}/correspondence.json")
add_custom_command(
    OUTPUT ${lifecycle_outputs}
    COMMAND "${Python3_EXECUTABLE}" "${lifecycle_generator}" --root "${lifecycle_root}" --output "${lifecycle_output}"
    DEPENDS "${lifecycle_generator}" "${CMAKE_CURRENT_SOURCE_DIR}/fixture.cpp.in"
        "${lifecycle_root}/Extension/ArcadeMode/arcade_mode.cpp"
        "${lifecycle_root}/Extension/ArcadeMode/arcade_balance_hud.cpp"
        "${lifecycle_root}/Extension/ArcadeMode/arcade_manual_acquisition.h"
    VERBATIM)
add_custom_target(dingosdk_arcade_manual_lifecycle_generate DEPENDS ${lifecycle_outputs})
foreach(variant IN LISTS lifecycle_variants)
    if(variant STREQUAL current)
        set(target dingosdk_arcade_manual_lifecycle_tests)
    else()
        set(target dingosdk_arcade_manual_lifecycle_${variant}_control)
    endif()
    add_executable(${target} "${lifecycle_output}/${variant}.cpp")
    add_dependencies(${target} dingosdk_arcade_manual_lifecycle_generate)
    target_include_directories(${target} PRIVATE "${lifecycle_root}")
    target_compile_features(${target} PRIVATE cxx_std_20)
    if(NOT variant STREQUAL current)
        add_dependencies(dingosdk_arcade_manual_lifecycle_tests ${target})
        add_test(NAME arcade_manual_lifecycle_rollback_${variant}
            COMMAND "${Python3_EXECUTABLE}" "${lifecycle_generator}"
                --expect-failure ${variant} --binary $<TARGET_FILE:${target}>)
        set_tests_properties(arcade_manual_lifecycle_rollback_${variant} PROPERTIES TIMEOUT 75)
    endif()
endforeach()
add_test(NAME arcade_manual_lifecycle COMMAND dingosdk_arcade_manual_lifecycle_tests)
add_test(NAME arcade_manual_lifecycle_source
    COMMAND "${Python3_EXECUTABLE}" "${lifecycle_generator}" --root "${lifecycle_root}" --output "${lifecycle_output}" --check)
set_tests_properties(arcade_manual_lifecycle arcade_manual_lifecycle_source PROPERTIES TIMEOUT 60)
