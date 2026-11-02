#pragma once
#include "arcade_trick_table.h"

namespace dingosdk::arcade_mode {
// Read-only interpretation of the supported build's Enum.SkateGrindType
// (75934470-c85c-df18-7593-4470c85cdf18). These IDs are neither input recipes
// nor permission to write native state. See GRIND_TRANSITIONS_2026_10_30.md.
enum class NativeGrindSide : std::uint8_t { unknown, frontside, backside };
struct NativeGrindIdentity {
    GrindChangeTarget target{};
    NativeGrindSide side{};
    bool bf{};
    std::uint32_t value{};
    constexpr explicit operator bool() const noexcept { return target != GrindChangeTarget::none; }
};
constexpr NativeGrindIdentity native_grind_identity(std::uint32_t value) noexcept {
    // Four explicitly enumerated blocks in the native asset, not stance
    // inferred from approach velocity or animation names.
    const bool bf = value >= 35 && value <= 68;
    const bool back = (value >= 18 && value <= 34) || (value >= 52 && value <= 68);
    if (!value || value > 68) return {};
    const auto offset = (bf ? 34u : 0u) + (back ? 17u : 0u);
    GrindChangeTarget target{};
    switch (value-offset) {
    case 3: target=GrindChangeTarget::grind_50_50; break;
    case 4: target=GrindChangeTarget::noseslide; break;
    case 5: target=GrindChangeTarget::tailslide; break;
    case 6: target=GrindChangeTarget::nosegrind; break;
    case 7: target=GrindChangeTarget::grind_5_0; break;
    case 9: target=GrindChangeTarget::smith; break;
    case 13: target=GrindChangeTarget::nosebluntslide; break;
    case 14: target=GrindChangeTarget::bluntslide; break;
    case 15: target=GrindChangeTarget::crooked; break;
    default: return {}; // Other native tricks do not alias these nine targets.
    }
    return {target,back ? NativeGrindSide::backside : NativeGrindSide::frontside,bf,value};
}
// Resolve a requested target in the current verified native identity's block.
// Zero means unresolved, including native Invalid/Exit and unsupported origins.
constexpr std::uint32_t native_grind_target(std::uint32_t current, GrindChangeTarget target) noexcept {
    const auto origin=native_grind_identity(current);
    if (!origin) return 0;
    std::uint32_t value{};
    switch (target) {
    case GrindChangeTarget::grind_50_50: value=3; break;
    case GrindChangeTarget::noseslide: value=4; break;
    case GrindChangeTarget::tailslide: value=5; break;
    case GrindChangeTarget::nosegrind: value=6; break;
    case GrindChangeTarget::grind_5_0: value=7; break;
    case GrindChangeTarget::smith: value=9; break;
    case GrindChangeTarget::nosebluntslide: value=13; break;
    case GrindChangeTarget::bluntslide: value=14; break;
    case GrindChangeTarget::crooked: value=15; break;
    default: return 0;
    }
    return value+(origin.bf ? 34u : 0u)+(origin.side==NativeGrindSide::backside ? 17u : 0u);
}
}
