#pragma once
#include <cstddef>
#include <cstdint>
namespace dingosdk::game::build::v20260929::native_scoring {
// Read-only Deck captures followed the original score HUD's ScoringData ref to
// this registered model. Field offsets are resolved through its native schema.
inline constexpr std::uint32_t schema = 0xa8b7e62f;
inline constexpr std::uint16_t size = 1280;
inline constexpr std::uint32_t sequence_schema = 0x5fbb7693;
inline constexpr std::uint16_t sequence_size = 528;
inline constexpr std::uint32_t line_schema = 0xf95bfe33;
inline constexpr std::uint16_t line_size = 40;
inline constexpr std::uint32_t current_sequence = 0x429eece6, previous_sequence = 0x893b9dff;
inline constexpr std::uint32_t current_line = 0xf99b169c, previous_line = 0x2da1d43e;
inline constexpr std::uint32_t line_sequences = 0xbcc10735, sequence_actions = 0x16c8ee36;
inline constexpr std::uint32_t sequence_points = 0x7d1e237c, sequence_index = 0x8c2ce177;
inline constexpr std::uint32_t sequence_kind = 0xa11c5161, current_action = 0x13e749e5;
inline constexpr std::uint32_t action_schema = 0xcc74a280;
// Native ArrayTypeInfo's element declaration, shared by both array types.
inline constexpr std::size_t array_element_type = 0x30;
}
