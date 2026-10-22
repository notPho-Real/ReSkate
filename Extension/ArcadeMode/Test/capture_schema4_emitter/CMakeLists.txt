cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ArcadeCaptureSchema4EmitterTests LANGUAGES CXX)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/../manual_test_warnings.cmake")
get_filename_component(capture_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
set(capture_generated "${CMAKE_CURRENT_BINARY_DIR}/generated/current.cpp")
set(capture_teardown_generated "${CMAKE_CURRENT_BINARY_DIR}/generated/teardown.cpp")
set(capture_generation_stamp "${CMAKE_CURRENT_BINARY_DIR}/generated/complete.stamp")
add_custom_command(OUTPUT "${capture_generation_stamp}"
    BYPRODUCTS "${capture_generated}" "${capture_teardown_generated}"
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/generate.py"
        --root "${capture_root}" --output "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${CMAKE_COMMAND}" -E touch "${capture_generation_stamp}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/generate.py" "${CMAKE_CURRENT_SOURCE_DIR}/fixture.cpp.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/teardown.cpp.in"
        "${capture_root}/Extension/ArcadeMode/arcade_mode.cpp"
    VERBATIM)
# One owner prevents parallel Visual Studio projects rerunning the same writer.
add_custom_target(dingosdk_arcade_capture_schema4_generate DEPENDS "${capture_generation_stamp}")
add_executable(dingosdk_arcade_capture_schema4_emitter_tests "${capture_generated}")
add_dependencies(dingosdk_arcade_capture_schema4_emitter_tests dingosdk_arcade_capture_schema4_generate)
dingosdk_arcade_manual_test_warnings(dingosdk_arcade_capture_schema4_emitter_tests)
target_include_directories(dingosdk_arcade_capture_schema4_emitter_tests PRIVATE "${capture_root}")
target_compile_features(dingosdk_arcade_capture_schema4_emitter_tests PRIVATE cxx_std_20)
add_test(NAME arcade_capture_schema4_emitter COMMAND dingosdk_arcade_capture_schema4_emitter_tests)
add_executable(dingosdk_arcade_capture_schema4_teardown_tests "${capture_teardown_generated}")
add_dependencies(dingosdk_arcade_capture_schema4_teardown_tests dingosdk_arcade_capture_schema4_generate)
dingosdk_arcade_manual_test_warnings(dingosdk_arcade_capture_schema4_teardown_tests)
target_include_directories(dingosdk_arcade_capture_schema4_teardown_tests PRIVATE "${capture_root}")
target_compile_features(dingosdk_arcade_capture_schema4_teardown_tests PRIVATE cxx_std_20)
find_package(Threads REQUIRED)
target_link_libraries(dingosdk_arcade_capture_schema4_teardown_tests PRIVATE Threads::Threads)
add_test(NAME arcade_capture_schema4_teardown COMMAND dingosdk_arcade_capture_schema4_teardown_tests)
add_test(NAME arcade_capture_schema4_emitter_source
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/generate.py"
        --root "${capture_root}" --output "${CMAKE_CURRENT_BINARY_DIR}/generated" --check)

add_test(NAME arcade_manual_lifecycle_capture_schema4_generation
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/generation_dependency_tests.py")
