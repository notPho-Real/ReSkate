cmake_minimum_required(VERSION 3.24)
project(DingoSDK LANGUAGES CXX C)
if(WIN32)
    if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "DingoSDK on Windows requires MSVC and the x64 platform.")
    endif()
    # ReSkate uses no vcpkg packages. A user-wide "vcpkg integrate install" still hooks every
    # project and runs its DLL copy after each link, with PowerShell 7 ("'pwsh.exe' is not
    # recognized" where it is not installed); the preset's VCLibPackagePath only covers configure.
    set(CMAKE_VS_GLOBALS "VCPkgLocalAppDataDisabled=true")
else()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "DingoSDK on Linux requires a 64-bit platform.")
    endif()
    message(STATUS "Linux build: dedicated server only (launcher/runtime stay Windows-only).")
endif()

include(cmake/ProjectOptions.cmake)
include(cmake/Dependencies.cmake)
include(cmake/Core.cmake)
if(WIN32)
    include(cmake/Features.cmake)
    include(cmake/UI.cmake)
    include(cmake/Runtime.cmake)
endif()
include(cmake/Apps.cmake)
if(WIN32)
    include(cmake/CrashReport.cmake)
endif()

option(DINGOSDK_BUILD_TESTS "Build logging regression tests" OFF)
if(DINGOSDK_BUILD_TESTS)
    enable_testing()
    add_executable(dingosdk_child_identity_tests Launcher/Test/child_identity_tests.cpp)
    target_include_directories(dingosdk_child_identity_tests PRIVATE "${PROJECT_SOURCE_DIR}")
    add_test(NAME child_identity COMMAND dingosdk_child_identity_tests)
    add_executable(dingosdk_rotating_log_tests Engine/Core/Log/Test/rotating_file_tests.cpp)
    target_include_directories(dingosdk_rotating_log_tests PRIVATE "${PROJECT_SOURCE_DIR}")
    add_test(NAME rotating_log COMMAND dingosdk_rotating_log_tests "${CMAKE_CURRENT_BINARY_DIR}/rotating-log-fixture")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(rotating_rollback_header "${CMAKE_CURRENT_BINARY_DIR}/rotating-log-rollback/Engine/Core/Log/rotating_file.h")
    set(rotating_rollback_script "${PROJECT_SOURCE_DIR}/Engine/Core/Log/Test/startup_rollback.py")
    add_custom_command(OUTPUT "${rotating_rollback_header}"
        COMMAND "${Python3_EXECUTABLE}" "${rotating_rollback_script}"
            --header "${PROJECT_SOURCE_DIR}/Engine/Core/Log/rotating_file.h" --output "${rotating_rollback_header}"
        DEPENDS "${rotating_rollback_script}" "${PROJECT_SOURCE_DIR}/Engine/Core/Log/rotating_file.h"
        VERBATIM)
    add_executable(dingosdk_rotating_log_startup_rollback Engine/Core/Log/Test/rotating_file_tests.cpp "${rotating_rollback_header}")
    target_include_directories(dingosdk_rotating_log_startup_rollback PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/rotating-log-rollback" "${PROJECT_SOURCE_DIR}")
    add_dependencies(dingosdk_rotating_log_tests dingosdk_rotating_log_startup_rollback)
    add_test(NAME rotating_log_startup_rollback COMMAND "${Python3_EXECUTABLE}" "${rotating_rollback_script}"
        --binary $<TARGET_FILE:dingosdk_rotating_log_startup_rollback>
        --fixture "${CMAKE_CURRENT_BINARY_DIR}/rotating-log-startup-rollback-fixture")
    set_tests_properties(rotating_log_startup_rollback PROPERTIES TIMEOUT 75)
endif()
