#pragma once
#include "Extension/Skater/no_bail.h"
#include <cstdint>
#include <optional>
namespace dingosdk::arcade_mode::native_flip {
bool start(std::uintptr_t base) noexcept;
void tick(bool gameplay) noexcept;
void cancel() noexcept;
// Reads the animation state without acquiring or changing a playback lease.
std::optional<int> observed_trick(const NoBailSkater& owner) noexcept;
bool available() noexcept;
void arm(const NoBailSkater& owner, bool from_rail=false) noexcept;
bool request(const NoBailSkater& owner, std::int32_t id) noexcept;
}
