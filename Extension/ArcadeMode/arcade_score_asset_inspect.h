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
    reference_limit = 1024, visit_limit = 8192, depth_limit = 8;
// Only fixed scoring-manager namespaces. No caller-provided asset path/filter.
bool allowed_asset(std::string_view name) noexcept;
// Declaration names are metadata; arbitrary string/scalar values are never exported.
Json inspect_document(const frostbite::ebx::Document& document);
Json targets();
}
