cmake_minimum_required(VERSION 3.24)
# Also usable without configuring the runtime or fetching its dependencies:
# cmake -S Extension/ArcadeMode/Test/manual_lifecycle -B <build-directory>
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ArcadeManualLifecycleTests LANGUAGES CXX)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/../manual_test_warnings.cmake")
find_package(Threads REQUIRED)
get_filename_component(lifecycle_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(lifecycle_output "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(lifecycle_generator "${CMAKE_CURRENT_SOURCE_DIR}/generate.py")
set(lifecycle_variants current model_reset producer_revoke buffered_release bail camera_toggle camera_recheck camera_publish camera_contention camera_snapshot_clock camera_client_phase)
set(lifecycle_sources)
foreach(variant IN LISTS lifecycle_variants)
    list(APPEND lifecycle_sources "${lifecycle_output}/${variant}.cpp")
endforeach()
set(lifecycle_outputs ${lifecycle_sources}
    "${lifecycle_output}/hud_source.cpp" "${lifecycle_output}/hud_axis.cpp" "${lifecycle_output}/hud_clear.cpp"
    "${lifecycle_output}/acquisition_without_bail.h" "${lifecycle_output}/correspondence.json")
add_custom_command(
    OUTPUT ${lifecycle_outputs}
    COMMAND "${Python3_EXECUTABLE}" "${lifecycle_generator}" --root "${lifecycle_root}" --output "${lifecycle_output}"
    DEPENDS "${lifecycle_generator}" "${CMAKE_CURRENT_SOURCE_DIR}/fixture.cpp.in"
        "${lifecycle_root}/Extension/ArcadeMode/arcade_mode.cpp"
        "${lifecycle_root}/Extension/ArcadeMode/arcade_balance_hud.cpp"
        "${lifecycle_root}/Extension/ArcadeMode/arcade_manual_acquisition.h"
    VERBATIM)
add_custom_target(dingosdk_arcade_manual_lifecycle_composition_generate DEPENDS ${lifecycle_outputs})
foreach(variant IN LISTS lifecycle_variants)
    if(variant STREQUAL current)
        set(target dingosdk_arcade_manual_lifecycle_composition_tests)
    else()
        set(target dingosdk_arcade_manual_lifecycle_composition_${variant}_control)
    endif()
    add_executable(${target} "${lifecycle_output}/${variant}.cpp")
    dingosdk_arcade_manual_test_warnings(${target})
    add_dependencies(${target} dingosdk_arcade_manual_lifecycle_composition_generate)
    target_include_directories(${target} PRIVATE "${lifecycle_root}")
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_link_libraries(${target} PRIVATE Threads::Threads)
    if(NOT variant STREQUAL current)
        add_dependencies(dingosdk_arcade_manual_lifecycle_composition_tests ${target})
        add_test(NAME arcade_manual_lifecycle_composition_rollback_${variant}
            COMMAND "${Python3_EXECUTABLE}" "${lifecycle_generator}"
                --expect-failure ${variant} --binary $<TARGET_FILE:${target}>)
        set_tests_properties(arcade_manual_lifecycle_composition_rollback_${variant} PROPERTIES TIMEOUT 75)
    endif()
endforeach()
# The portable composition target uses only ImGui core; no window, renderer
# backend, runtime dependency download or platform SDK is needed.
if(TARGET dingosdk_imgui)
    set(lifecycle_imgui dingosdk_imgui)
else()
    add_library(dingosdk_arcade_hud_test_imgui STATIC
        "${lifecycle_root}/External/imgui/imgui.cpp"
        "${lifecycle_root}/External/imgui/imgui_draw.cpp"
        "${lifecycle_root}/External/imgui/imgui_tables.cpp"
        "${lifecycle_root}/External/imgui/imgui_widgets.cpp")
    target_include_directories(dingosdk_arcade_hud_test_imgui PUBLIC "${lifecycle_root}/External/imgui")
    target_compile_features(dingosdk_arcade_hud_test_imgui PRIVATE cxx_std_20)
    set(lifecycle_imgui dingosdk_arcade_hud_test_imgui)
endif()
foreach(variant current hud_source hud_axis hud_clear)
    if(variant STREQUAL current)
        set(target dingosdk_arcade_manual_lifecycle_composition_hud_tests)
    else()
        set(target dingosdk_arcade_manual_lifecycle_composition_${variant}_control)
    endif()
    add_executable(${target} "${lifecycle_output}/${variant}.cpp")
    dingosdk_arcade_manual_test_warnings(${target})
    add_dependencies(${target} dingosdk_arcade_manual_lifecycle_composition_generate)
    target_include_directories(${target} PRIVATE "${lifecycle_root}")
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_link_libraries(${target} PRIVATE Threads::Threads)
    target_compile_definitions(${target} PRIVATE DINGOSDK_MANUAL_LIFECYCLE_HUD)
    target_link_libraries(${target} PRIVATE ${lifecycle_imgui})
    add_dependencies(dingosdk_arcade_manual_lifecycle_composition_tests ${target})
    if(NOT variant STREQUAL current)
        add_test(NAME arcade_manual_lifecycle_composition_rollback_${variant}
            COMMAND "${Python3_EXECUTABLE}" "${lifecycle_generator}"
                --expect-failure ${variant} --binary $<TARGET_FILE:${target}>)
        set_tests_properties(arcade_manual_lifecycle_composition_rollback_${variant} PROPERTIES TIMEOUT 75)
    endif()
endforeach()
add_test(NAME arcade_manual_lifecycle_composition_hud COMMAND dingosdk_arcade_manual_lifecycle_composition_hud_tests)
set_tests_properties(arcade_manual_lifecycle_composition_hud PROPERTIES TIMEOUT 60)
add_test(NAME arcade_manual_lifecycle_composition COMMAND dingosdk_arcade_manual_lifecycle_composition_tests)
add_test(NAME arcade_manual_lifecycle_composition_source
    COMMAND "${Python3_EXECUTABLE}" "${lifecycle_generator}" --root "${lifecycle_root}" --output "${lifecycle_output}" --check)
set_tests_properties(arcade_manual_lifecycle_composition arcade_manual_lifecycle_composition_source PROPERTIES TIMEOUT 60)
