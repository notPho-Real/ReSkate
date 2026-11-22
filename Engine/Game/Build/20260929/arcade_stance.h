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
// The live asset prefix contains its exported instance GUID, not the file GUID
// of the EBX container. Records 1014/818/407/22455 distinguish these identities;
// InputScheme imports independently corroborate the PIA and processor objects.
inline constexpr std::array<Asset,4> assets{{
    {"Configurations/Input/DingoEcs_InputSchemeConfig",{0xd2,0x4a,0x46,0x5d,0x17,0x48,0x19,0x4b,0xb5,0xa1,0x37,0x24,0x2f,0xca,0xa8,0x9b}},
    {"Configurations/Input/PIA_Gameplay_ChangeStance",{0xee,0xbc,0xfd,0x55,0x7e,0x89,0xf3,0x4f,0x87,0xf2,0xe6,0x84,0x1f,0x26,0x4e,0x04}},
    {"Configurations/Input/Processor_Pad_B_DoubleTap",{0x73,0x6d,0x5b,0xa6,0x5d,0xb8,0x8d,0x40,0x95,0xbd,0xbf,0x0e,0x2b,0xc8,0x85,0xa4}},
    {"Gameplay/Input/Inputs/Input_ChangeStance",{0x79,0x77,0x05,0xfe,0x62,0x12,0x5f,0x42,0x8e,0x4c,0xb9,0x83,0xcf,0x08,0xed,0xf1}}
}};
}
