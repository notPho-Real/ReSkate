cmake_minimum_required(VERSION 3.24)
# Portable source composition; no runtime or dependency downloads are needed.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ArcadeManualReservationConsumerTests LANGUAGES CXX)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/../manual_test_warnings.cmake")
get_filename_component(reservation_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(reservation_generator "${CMAKE_CURRENT_SOURCE_DIR}/generate.py")
set(reservation_output "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${reservation_output}")
foreach(variant current rollback-no-reservation rollback-admitted-only rollback-first-down-mask rollback-native-axis)
    set(source "${reservation_output}/${variant}.cpp")
    set(target "dingosdk_arcade_manual_reservation_${variant}")
    add_custom_command(OUTPUT "${source}" "${reservation_output}/${variant}.json"
        COMMAND "${Python3_EXECUTABLE}" "${reservation_generator}"
            --root "${reservation_root}" --output "${source}" --variant "${variant}"
        DEPENDS "${reservation_generator}"
            "${reservation_root}/Extension/ArcadeMode/Test/manual_lifecycle/generate.py"
            "${reservation_root}/Extension/ArcadeMode/Test/manual_lifecycle/fixture.cpp.in"
            "${reservation_root}/Extension/ArcadeMode/arcade_mode.cpp"
            "${reservation_root}/Extension/ArcadeMode/arcade_balance_hud.cpp"
            "${reservation_root}/Extension/ArcadeMode/arcade_manual_acquisition.h"
            "${reservation_root}/Extension/ArcadeMode/arcade_ground_motion_step.h"
            "${reservation_root}/Extension/Skater/client_noclip.cpp"
        VERBATIM)
    add_executable(${target} "${source}")
    dingosdk_arcade_manual_test_warnings(${target})
    target_include_directories(${target} PRIVATE "${reservation_root}")
    target_compile_features(${target} PRIVATE cxx_std_20)
    if(variant STREQUAL current)
        add_test(NAME arcade_manual_reservation_consumer COMMAND ${target})
    else()
        add_dependencies(dingosdk_arcade_manual_reservation_current ${target})
        add_test(NAME "arcade_manual_reservation_${variant}"
            COMMAND "${Python3_EXECUTABLE}" "${reservation_generator}"
                --verify-control "${variant}" --binary $<TARGET_FILE:${target}>)
    endif()
    add_test(NAME "arcade_manual_reservation_source_${variant}"
        COMMAND "${Python3_EXECUTABLE}" "${reservation_generator}"
            --root "${reservation_root}" --output "${source}" --variant "${variant}" --check)
    set_tests_properties("arcade_manual_reservation_source_${variant}" PROPERTIES TIMEOUT 60)
endforeach()
set_tests_properties(arcade_manual_reservation_consumer
    arcade_manual_reservation_rollback-no-reservation
    arcade_manual_reservation_rollback-admitted-only arcade_manual_reservation_rollback-first-down-mask
    arcade_manual_reservation_rollback-native-axis PROPERTIES TIMEOUT 60)
