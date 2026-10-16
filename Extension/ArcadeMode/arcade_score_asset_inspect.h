#pragma once
#include "Engine/Core/Json/json.h"
#include "Engine/Resource/ebx_document.h"
#include <array>
#include <string_view>

namespace dingosdk::arcade_mode::score_assets {
inline constexpr std::string_view toc_name = "Win32/levels/game/bam_levelroot/bam_levelroot.toc";
inline constexpr std::array bundle_names{
    "win32/levels/game/bam_levelroot/bam_levelroot",
    "win32/levels/game/bam_levelroot/bam_coregameassets"};
inline constexpr std::size_t asset_limit = 32, payload_limit = 2 * 1024 * 1024,
    total_payload_limit = 32 * 1024 * 1024, output_limit = 2 * 1024 * 1024;
inline constexpr std::size_t type_limit = 256, field_limit = 2048, instance_limit = 512,
    reference_limit = 1024, visit_limit = 8192, depth_limit = 8, declared_member_limit = 128;
inline constexpr std::size_t manifest_name_limit = 256;
struct DeclarationTarget { std::string_view role, file_guid, class_guid; };
// Exact imports observed in the pinned game's scoring declarations.
inline constexpr std::array declaration_targets{
    DeclarationTarget{"shared_graph_parameter", "c883e9c8-ec6a-11ee-a230-98e8e08ce66d", "c883e9c8-ec6a-11ee-a230-98e8e08ce66d"},
    DeclarationTarget{"line_data", "d8fb28fb-d3fb-11ed-bee3-9c0b005d0b30", "4fc02350-0fea-216e-55ad-0ba72ea3da4c"},
    DeclarationTarget{"sequence_data", "a515778d-ad7c-11ed-bff6-dff778381e5f", "f607ab87-140d-528a-bb70-6c73565cb199"},
    DeclarationTarget{"local_player_spawn_value", "a623e6bf-3275-11ef-a261-f9f727c65ba1", "40c4614e-cae7-11bf-2b04-4504f690dfb8"}};
bool matches_declaration(const frostbite::ebx::Document& document, const DeclarationTarget& target);
// Only fixed scoring-manager namespaces. No caller-provided asset path/filter.
bool allowed_asset(std::string_view name) noexcept;
// Rank known graph/event declarations before the bounded namespace remainder.
unsigned asset_priority(std::string_view name) noexcept;
struct AssetCandidate {
    std::string_view name;
    std::size_t bundle{}, index{};
    bool resource{};
    bool exact_spawn_candidate{};
};
// The token comes from the captured callback name. This is a manifest-name
// candidate only; both imported GUIDs must match before resolving it.
bool exact_spawn_name(std::string_view name) noexcept;
struct SpawnDeclarationCandidates {
    std::array<AssetCandidate, 4> candidates{};
    std::size_t matches{}, size{};
    void consider(AssetCandidate candidate) noexcept;
};
// Views refer to the caller's retained bundle manifests. Storage stays at 32.
struct AssetSelection {
    std::array<AssetCandidate, asset_limit> assets{};
    std::size_t size{};
    bool omitted{};
    void consider(AssetCandidate candidate) noexcept;
};
// Manifest names only: no additional payload reads or namespace expansion.
struct DeclarationInventory {
    std::array<AssetCandidate, manifest_name_limit> assets{};
    std::size_t size{};
    bool omitted{};
    void consider(AssetCandidate candidate) noexcept;
};
// Only declared member names/hashes join metadata; arbitrary values stay omitted.
Json inspect_document(const frostbite::ebx::Document& document);
Json targets();
}
