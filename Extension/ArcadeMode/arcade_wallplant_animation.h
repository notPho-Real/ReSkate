#pragma once
#include "Extension/Skater/no_bail.h"
#include <cstdint>

namespace dingosdk::arcade_mode::wallplant_animation {
// Client thread only, like skater_animation::play. Preload is asynchronous and
// never reads game memory on its worker. Missing clips leave admission closed.
void tick(bool gameplay) noexcept;
bool available(const NoBailSkater& owner) noexcept;
// The caller has already verified contact, input and rebound. This plays an
// original planted-foot clip through the existing guarded pose facility. It is
// a visual approximation, not a native Footplant state or scoring occurrence.
bool request(const NoBailSkater& owner, std::uint64_t occurrence) noexcept;
void cancel() noexcept;
}
