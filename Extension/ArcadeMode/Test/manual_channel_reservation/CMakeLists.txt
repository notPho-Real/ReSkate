cmake_minimum_required(VERSION 3.24)
# Portable source composition only; no runtime or dependency downloads.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ArcadeManualChannelReservationTests LANGUAGES CXX)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/../manual_test_warnings.cmake")
get_filename_component(channel_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(channel_generator "${CMAKE_CURRENT_SOURCE_DIR}/generate.py")
set(channel_output "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${channel_output}")
foreach(variant current rollback-owner rollback-reservation rollback-competitor rollback-rs rollback-release)
    set(source "${channel_output}/${variant}.cpp")
    set(target "dingosdk_arcade_manual_channel_${variant}")
    add_custom_command(OUTPUT "${source}" "${channel_output}/${variant}.json"
        COMMAND "${Python3_EXECUTABLE}" "${channel_generator}"
            --root "${channel_root}" --output "${source}" --variant "${variant}"
        DEPENDS "${channel_generator}" "${CMAKE_CURRENT_SOURCE_DIR}/fixture.cpp.in"
            "${channel_root}/Extension/ArcadeMode/arcade_mode.cpp"
            "${channel_root}/Extension/Skater/client_noclip.cpp"
        VERBATIM)
    add_executable(${target} "${source}")
    dingosdk_arcade_manual_test_warnings(${target})
    target_include_directories(${target} PRIVATE "${channel_root}")
    target_compile_features(${target} PRIVATE cxx_std_20)
    if(variant STREQUAL current)
        add_test(NAME arcade_manual_lifecycle_channel_reservation COMMAND ${target})
    else()
        add_dependencies(dingosdk_arcade_manual_channel_current ${target})
        add_test(NAME "arcade_manual_lifecycle_channel_${variant}"
            COMMAND "${Python3_EXECUTABLE}" "${channel_generator}"
                --verify-control "${variant}" --binary $<TARGET_FILE:${target}>)
    endif()
    add_test(NAME "arcade_manual_lifecycle_channel_source_${variant}"
        COMMAND "${Python3_EXECUTABLE}" "${channel_generator}"
            --root "${channel_root}" --output "${source}" --variant "${variant}" --check)
    set_tests_properties("arcade_manual_lifecycle_channel_source_${variant}" PROPERTIES TIMEOUT 60)
endforeach()
# The existing preflight composition target must build both real publication
# and scalar-consumer controls. Lifecycle-prefixed tests enter its CTest filter.
if(TARGET dingosdk_arcade_manual_lifecycle_composition_tests)
    add_dependencies(dingosdk_arcade_manual_lifecycle_composition_tests dingosdk_arcade_manual_channel_current)
    if(TARGET dingosdk_arcade_manual_reservation_current)
        add_dependencies(dingosdk_arcade_manual_lifecycle_composition_tests dingosdk_arcade_manual_reservation_current)
        set(consumer_generator "${channel_root}/Extension/ArcadeMode/Test/manual_reservation_consumer/generate.py")
        foreach(variant current rollback-no-reservation rollback-admitted-only)
            set(target "dingosdk_arcade_manual_reservation_${variant}")
            get_target_property(source ${target} SOURCES)
            if(variant STREQUAL current)
                add_test(NAME arcade_manual_lifecycle_reservation_consumer COMMAND ${target})
            else()
                add_test(NAME "arcade_manual_lifecycle_reservation_consumer_${variant}"
                    COMMAND "${Python3_EXECUTABLE}" "${consumer_generator}"
                        --verify-control "${variant}" --binary $<TARGET_FILE:${target}>)
            endif()
            add_test(NAME "arcade_manual_lifecycle_reservation_consumer_source_${variant}"
                COMMAND "${Python3_EXECUTABLE}" "${consumer_generator}"
                    --root "${channel_root}" --output "${source}" --variant "${variant}" --check)
            set_tests_properties("arcade_manual_lifecycle_reservation_consumer_source_${variant}" PROPERTIES TIMEOUT 60)
        endforeach()
    endif()
endif()
