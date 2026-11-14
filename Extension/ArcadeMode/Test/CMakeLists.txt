cmake_minimum_required(VERSION 3.24)
project(ArcadeModeTests LANGUAGES CXX)

enable_testing()
get_filename_component(reskate_root "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
include(manual_test_warnings.cmake)

# Compile the production headers without starting the root runtime/launcher build
# or depending on the fork's CI scripts. Native execution still needs Windows
# and independent gameplay acceptance; these tests cover the portable contracts.
foreach(test camera_input input_session neutral_flip_release air_motion_proof launch_descriptor
             input_pattern_dispatch manual_step manual_balance_entry
             native_spin_integration native_readiness rail_pop
             grind_transition_lifetime grind_change native_grab manual_throw
             native_orientation playback held_flip manual footplant ground_grab
             fingerflip trickpedia trickpedia_lifetime wallplant contact_owner
             control_state input_consolidation)
    add_executable(arcade_${test} "arcade_${test}_tests.cpp")
    target_include_directories(arcade_${test} PRIVATE "${reskate_root}")
    dingosdk_arcade_manual_test_warnings(arcade_${test})
    add_test(NAME arcade_${test} COMMAND arcade_${test})
endforeach()

# Link the real disabled implementation: a native dependency here must fail the
# portable build instead of silently shipping with wallplants disabled.
add_executable(arcade_wallplant_disabled
    arcade_wallplant_disabled_tests.cpp ../arcade_wallplant_runtime.cpp)
target_include_directories(arcade_wallplant_disabled PRIVATE "${reskate_root}")
dingosdk_arcade_manual_test_warnings(arcade_wallplant_disabled)
add_test(NAME arcade_wallplant_disabled COMMAND arcade_wallplant_disabled)

add_subdirectory(custom_flatland)
