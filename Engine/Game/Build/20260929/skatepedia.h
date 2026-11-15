#pragma once
#include <cstdint>

namespace dingosdk::game::build::v20260929::skatepedia {
// Native type declarations and read-only live card observations on this build.
// The page owns navigation and previews; only PIALabel holds input guidance.
inline constexpr std::uint32_t info_card_schema=0xa8080640;
inline constexpr std::uint16_t info_card_size=248;
inline constexpr std::uint32_t inputs=0xd5254e96, title=0xe08a935d, text=0x4d8e01b9;
inline constexpr std::uint32_t focus=0xfafda1a6, focus_state=0x20a39d92, focus_index=0x6b01fd01;
inline constexpr std::uint32_t back=0x10810f0b, button=0x0fb0d794, action=0xa704272a,
    action_state=0xc52416ef, navigation=0xa2b71c93, navigation_name=0xb1e14cd6;
}
