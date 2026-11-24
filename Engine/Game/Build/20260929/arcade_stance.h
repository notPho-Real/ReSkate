#pragma once
#include "arcade_flip_transition.h"
#include "arcade_held_flip.h"
#include "launches.h"
#include "arcade_fingerflip.h"

namespace dingosdk::game::build::v20260929::arcade_stance {
using Guid=arcade_held_flip::Guid;
// Resolved from the original Regular -> ChangeStance expression in a live
// supported-build capture. This consumes the predicate; native EAT still owns
// branch selection, timing, initialization and cancellation.
inline constexpr std::uintptr_t transition_consumer=0x2d56610;
inline constexpr std::array<unsigned char,32> transition_consumer_prefix{
    0x40,0x53,0x55,0x56,0x57,0x48,0x83,0xec,0x48,0x48,0x8b,0x19,0x41,0x0f,0xb6,0x28,
    0x44,0x0f,0xb6,0x02,0xc5,0xf8,0x29,0x74,0x24,0x30,0xc5,0xfa,0x10,0x71,0x08,0x4c};
inline constexpr std::uint32_t regular_transition_key=0xfae98387;
inline constexpr std::array<std::uint32_t,10> regular_transition_layout{
    64,200,308,399,5,0,0,327718,2162688,0};
inline constexpr std::size_t transition_context_page=24, transition_predicate_page=16,
    transition_predicate_offset=307, transition_flags_page=0, transition_flags_offset=192;
// Read-only execution evidence from the pinned consumer: context[0] owns the
// action, action+8 tags its data pointer, +60 stores the resolved branch and
// +11 remains set when native transition admission has not completed.
inline constexpr std::size_t consumer_action=0, action_data=8, action_pending=0x11,
    action_branch=0x60, action_elapsed=0x68, action_truth_elapsed=0x6c,
    data_truth_duration=0x60, data_transition_time=0x64;
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
inline constexpr Asset transition_asset{
    "Gameplay/SkaterIntentTree/Default_Regular_AG_9B658BD_EATTransitionActionData_4_ss_OnUpdate",
    {0x6f,0xbe,0x0f,0xcf,0x8c,0x72,0x4e,0xc9,0x0d,0xab,0x45,0xa5,0x92,0xa0,0x71,0xd1}};
inline constexpr std::size_t expression_graph=152, graph_resource_wrapper=72, wrapper_resource=56;
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
