# Evidence can be added after testing without invalidating itself. Any change to
# gameplay/engine source or canonical bindings invalidates prior acceptance.
file(GLOB_RECURSE arcade_evidence_sources CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/Extension/ArcadeMode/*.h"
    "${PROJECT_SOURCE_DIR}/Extension/ArcadeMode/*.cpp"
    "${PROJECT_SOURCE_DIR}/Extension/ArcadeMode/*.inl"
    "${PROJECT_SOURCE_DIR}/Engine/*.h"
    "${PROJECT_SOURCE_DIR}/Engine/*.cpp"
    "${PROJECT_SOURCE_DIR}/Engine/*.inl")
list(FILTER arcade_evidence_sources EXCLUDE REGEX "/Test/")
list(FILTER arcade_evidence_sources EXCLUDE REGEX "/arcade_(trick_evidence|trick_capture_data|trick_acceptance)\\.inl$")
list(FILTER arcade_evidence_sources EXCLUDE REGEX "/arcade_(evidence_types|trickpedia|registry_validation)\\.h$")
list(SORT arcade_evidence_sources)
set(arcade_source_manifest "")
foreach(source IN LISTS arcade_evidence_sources)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source}")
    file(RELATIVE_PATH relative "${PROJECT_SOURCE_DIR}" "${source}")
    # Git checkout line endings must not create different Windows/Linux receipts.
    file(READ "${source}" source_text)
    string(REPLACE "\r\n" "\n" source_text "${source_text}")
    string(SHA256 digest "${source_text}")
    string(APPEND arcade_source_manifest "${relative}:${digest}\n")
endforeach()
string(SHA256 arcade_gameplay_fingerprint "${arcade_source_manifest}")
foreach(target dingosdk_runtime dingosdk_overlay)
    target_compile_definitions(${target} PRIVATE "DINGOSDK_ARCADE_GAMEPLAY_FINGERPRINT=\"${arcade_gameplay_fingerprint}\"")
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/arcade-gameplay-fingerprint.txt" "${arcade_gameplay_fingerprint}\n")
