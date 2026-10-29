# MSBuild treats CMAKE_CONFIGURE_DEPENDS entries as files, not directories.
# CONFIGURE_DEPENDS detects added/removed optional assets; existing files also
# trigger regeneration when changed. An absent asset directory is valid.
file(GLOB_RECURSE dingosdk_optional_assets CONFIGURE_DEPENDS LIST_DIRECTORIES false
    "${PROJECT_SOURCE_DIR}/assets/*.png" "${PROJECT_SOURCE_DIR}/assets/*.jpg"
    "${PROJECT_SOURCE_DIR}/assets/*.json" "${PROJECT_SOURCE_DIR}/assets/*.ico")
if(dingosdk_optional_assets)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${dingosdk_optional_assets})
endif()
