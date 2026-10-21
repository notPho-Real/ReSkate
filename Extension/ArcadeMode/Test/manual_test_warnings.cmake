include_guard(GLOBAL)
# Warning policy only: safe in the root build and standalone test projects.
# Do not include runtime paths, source grouping or third-party library targets.
function(dingosdk_arcade_manual_test_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /EHsc)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wshadow -Wconversion -Wsign-conversion -Werror)
    else()
        message(FATAL_ERROR "Manual test warning policy is unavailable for this compiler")
    endif()
endfunction()
