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
// Only fixed scoring-manager namespaces. No caller-provided asset path/filter.
bool allowed_asset(std::string_view name) noexcept;
// Rank known graph/event declarations before the bounded namespace remainder.
unsigned asset_priority(std::string_view name) noexcept;
struct AssetCandidate {
    std::string_view name;
    std::size_t bundle{}, index{};
    bool resource{};
};
// Views refer to the caller's retained bundle manifests. Storage stays at 32.
struct AssetSelection {
    std::array<AssetCandidate, asset_limit> assets{};
    std::size_t size{};
    bool omitted{};
    void consider(AssetCandidate candidate) noexcept;
};
// Only declared member names/hashes join metadata; arbitrary values stay omitted.
Json inspect_document(const frostbite::ebx::Document& document);
Json targets();
}
