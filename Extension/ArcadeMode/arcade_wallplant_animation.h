#pragma once
#include "Extension/Skater/no_bail.h"
#include <cstdint>
#include <array>

namespace dingosdk::arcade_mode::wallplant_animation {
// Client thread only, like skater_animation::play. Preload is asynchronous and
// never reads game memory on its worker. Missing clips leave admission closed.
void tick(bool gameplay) noexcept;
bool available(const NoBailSkater& owner) noexcept;
// Client-only, after a successful authenticated physical entry brake. The
// caller retains live hold authority; native100 is allowed only for this owned
// visual phase. No native state, root, contact, or scoring write is performed.
bool begin_hold(const NoBailSkater& owner,std::uint64_t occurrence,std::uint64_t started_at,std::uint64_t expires_at,
    const std::array<float,3>& wall_point,const std::array<float,3>& wall_normal,
    const std::array<float,3>& outgoing_forward) noexcept;
// Actual physical release or cancellation ends only the matching borrowed play.
void release(const NoBailSkater& owner,std::uint64_t occurrence) noexcept;
void cancel() noexcept;
}
