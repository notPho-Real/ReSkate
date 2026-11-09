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
# Existing production-header tests, not copied implementations. No runtime DLL,
# asset downloads, Cargo, package or publishing permissions in this fast lane.
foreach(test input_session neutral_flip_release air_motion_proof launch_descriptor
             input_pattern_dispatch manual_step manual_balance_entry native_spin_integration native_readiness rail_pop
             grind_transition_lifetime grind_change native_grab manual_throw native_orientation playback held_flip manual footplant ground_grab fingerflip trickpedia trickpedia_lifetime
             wallplant contact_owner control_state input_consolidation)
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
