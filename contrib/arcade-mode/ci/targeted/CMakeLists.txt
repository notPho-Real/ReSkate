cmake_minimum_required(VERSION 3.24)
project(ArcadeTargetedTests LANGUAGES CXX)
get_filename_component(reskate_root "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
enable_testing()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
# Catch workflow selection and candidate feature mismatches before a DLL build.
add_test(NAME arcade_settings_snapshot_source
    COMMAND "${Python3_EXECUTABLE}" -B
        "${reskate_root}/Extension/ArcadeMode/Test/settings_snapshot_source_checks.py")
add_test(NAME arcade_unified_candidate
    COMMAND "${Python3_EXECUTABLE}" -B
        "${reskate_root}/contrib/arcade-mode/ci/test_unified_candidate.py")
# Existing production-header tests, not copied implementations. No runtime DLL,
# asset downloads, Cargo, package or publishing permissions in this fast lane.
foreach(test vert_air native_score_snapshot native_score_progress native_score_presentation native_score_rows custom_score input_session neutral_flip_release air_motion_proof launch_descriptor
             input_pattern_dispatch manual_step manual_balance_entry native_spin_integration native_readiness rail_pop
             grind_transition_lifetime grind_change native_grab manual_throw native_orientation playback held_flip manual footplant ground_grab fingerflip trickpedia trickpedia_lifetime
             wallplant wallplant_hold wallplant_combo contact_owner static_world_owner control_state input_consolidation)
    add_executable(arcade_${test} "${reskate_root}/Extension/ArcadeMode/Test/arcade_${test}_tests.cpp")
    target_include_directories(arcade_${test} PRIVATE "${reskate_root}")
    if(MSVC)
        # Match the full preflight so template warnings fail in this fast lane.
        target_compile_options(arcade_${test} PRIVATE /W4 /WX /permissive- /EHsc)
    endif()
    add_test(NAME arcade_${test} COMMAND arcade_${test})
endforeach()
add_executable(arcade_wallplant_disabled
    "${reskate_root}/Extension/ArcadeMode/Test/arcade_wallplant_disabled_tests.cpp"
    "${reskate_root}/Extension/ArcadeMode/arcade_wallplant_runtime.cpp")
target_include_directories(arcade_wallplant_disabled PRIVATE "${reskate_root}")
if(MSVC)
    target_compile_options(arcade_wallplant_disabled PRIVATE /W4 /WX /permissive- /EHsc)
endif()
add_test(NAME arcade_wallplant_disabled COMMAND arcade_wallplant_disabled)

add_subdirectory("${reskate_root}/Extension/ArcadeMode/Test/custom_flatland" custom_flatland)

# Exercise the integrated enabled runtime as well as its disabled counterpart.
add_executable(arcade_wallplant_runtime
    "${reskate_root}/Extension/ArcadeMode/Test/arcade_wallplant_runtime_tests.cpp")
set_property(TARGET arcade_wallplant_runtime PROPERTY CXX_STANDARD 23)
target_include_directories(arcade_wallplant_runtime BEFORE PRIVATE
    "${reskate_root}/Extension/ArcadeMode/Test/RuntimeBoundary" "${reskate_root}")
target_compile_definitions(arcade_wallplant_runtime PRIVATE DINGOSDK_ARCADE_WALLPLANTS)
add_test(NAME arcade_wallplant_runtime COMMAND arcade_wallplant_runtime)
add_executable(arcade_wallplant_animation
    "${reskate_root}/Extension/ArcadeMode/Test/arcade_wallplant_animation_tests.cpp"
    "${reskate_root}/Extension/Skater/arm_reach.cpp")
target_include_directories(arcade_wallplant_animation PRIVATE "${reskate_root}")
target_link_libraries(arcade_wallplant_animation PRIVATE casper_animation_core)
add_test(NAME arcade_wallplant_animation COMMAND arcade_wallplant_animation)
if(MSVC)
    target_compile_options(arcade_wallplant_runtime PRIVATE /W4 /WX /permissive- /EHsc)
    target_compile_options(arcade_wallplant_animation PRIVATE /W4 /WX /permissive- /EHsc)
endif()
