set(generator "${CMAKE_CURRENT_SOURCE_DIR}/generate.py")
set(fixture "${CMAKE_CURRENT_BINARY_DIR}/ground_writer.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${generator}" "${CMAKE_CURRENT_SOURCE_DIR}/fixture.cpp.in"
    "${PROJECT_SOURCE_DIR}/Extension/Skater/client_noclip.cpp" "${PROJECT_SOURCE_DIR}/Extension/ArcadeMode/arcade_mode.cpp")
execute_process(COMMAND "${Python3_EXECUTABLE}" -B "${generator}" --root "${PROJECT_SOURCE_DIR}" --output "${fixture}"
    RESULT_VARIABLE generated)
if(NOT generated EQUAL 0)
    message(FATAL_ERROR "Ground writer source fixture generation failed")
endif()
add_executable(dingosdk_arcade_ground_writer_tests "${fixture}")
include("${CMAKE_CURRENT_SOURCE_DIR}/../manual_test_warnings.cmake")
dingosdk_arcade_manual_test_warnings(dingosdk_arcade_ground_writer_tests)
target_include_directories(dingosdk_arcade_ground_writer_tests PRIVATE "${PROJECT_SOURCE_DIR}")
target_compile_features(dingosdk_arcade_ground_writer_tests PRIVATE cxx_std_20)
find_package(Threads REQUIRED)
target_link_libraries(dingosdk_arcade_ground_writer_tests PRIVATE Threads::Threads)
add_test(NAME arcade_ground_writer COMMAND dingosdk_arcade_ground_writer_tests)
add_test(NAME arcade_ground_writer_source COMMAND "${Python3_EXECUTABLE}" -B "${generator}"
    --root "${PROJECT_SOURCE_DIR}" --output "${fixture}" --check)
