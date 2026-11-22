#pragma once
#include "arcade_flip_transition.h"
#include "arcade_held_flip.h"
#include "launches.h"

namespace dingosdk::game::build::v20260929::arcade_stance {
using Guid=arcade_held_flip::Guid;
inline constexpr auto domain_owners=arcade_flip_transition::domain_owners;
inline constexpr auto physics_state=arcade_held_flip::physics_state;
inline constexpr auto stance_flags=launches::stance_flags_offset;
inline constexpr std::uintptr_t asset_guid_prefix=16;
inline constexpr std::uint16_t button=0x2000;
// The configured ChangeStance PIA uses Processor_Pad_B_DoubleTap, not the
// ordinary B processor: digital Button7, MatchType9, OutputType1, one button,
// DoublePressTime0.3 and HoldTime0 in the supported build's original assets.
// Identifiers and our notes only; no extracted asset data is stored here.
inline constexpr std::uint64_t double_press_ms=300;
struct Asset {const char* name;Guid guid;};
inline constexpr std::array<Asset,4> assets{{
    {"Configurations/Input/DingoEcs_InputSchemeConfig",{0x1a,0xae,0xa6,0x40,0x29,0xbd,0xb2,0x4a,0x97,0xfb,0x22,0x68,0xc9,0x8b,0x56,0x0e}},
    {"Configurations/Input/PIA_Gameplay_ChangeStance",{0xd2,0x48,0xbb,0x3d,0x7b,0xbe,0x32,0x4a,0xa8,0xee,0x92,0x3e,0x5d,0x61,0x14,0xfa}},
    {"Configurations/Input/Processor_Pad_B_DoubleTap",{0xa4,0xa7,0xc8,0x25,0x03,0xd7,0x27,0x4e,0x9b,0x97,0x9e,0x66,0xd1,0x61,0x1a,0x32}},
    {"Gameplay/Input/Inputs/Input_ChangeStance",{0x2f,0xed,0xa2,0x9e,0x6d,0xbf,0xdd,0x4a,0x80,0x0f,0x3b,0x7c,0x1b,0x9b,0xcb,0x46}}
}};
}
