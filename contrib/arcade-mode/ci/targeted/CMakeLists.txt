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
             manual_step manual_balance_entry native_spin_integration native_readiness rail_pop)
    add_executable(arcade_${test} "${reskate_root}/Extension/ArcadeMode/Test/arcade_${test}_tests.cpp")
    target_include_directories(arcade_${test} PRIVATE "${reskate_root}")
    add_test(NAME arcade_${test} COMMAND arcade_${test})
endforeach()
