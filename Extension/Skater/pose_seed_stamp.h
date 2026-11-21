#pragma once
#include <cmath>
#include <cstdint>

namespace dingosdk::skater_animation {
// Identity of a read-only seed, not a grant to write a pose or a collision body.
struct PoseSeedStamp {
    std::uintptr_t base{}, entity{}, component{}, board_holder{};
    std::uint64_t generation{};
    bool operator==(const PoseSeedStamp&) const = default;
};
inline bool pose_seed_current(const PoseSeedStamp& sample, const PoseSeedStamp& owner,
        double at, double now) noexcept {
    return sample.base && sample.entity && sample.component && sample.board_holder &&
        sample == owner && std::isfinite(at) && std::isfinite(now) && now >= at && now-at < .15;
}
}
