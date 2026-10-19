#pragma once
#include "arcade_combo_session.h"
#include <cstdint>

namespace dingosdk::arcade_mode {
// Putdown delivery and observed physical completion are separate. A model
// endpoint cannot establish an exit, and an unknown contact cannot complete it.
class ManualRelease {
public:
    struct Output { bool pending{}, completed{}, started{}; };
    void reset() noexcept { pending_ = requested_ = clocked_ = false; }
    Output step(std::uint64_t now, std::uint64_t owner_epoch, bool valid,
                SkatePose physical, bool putdown) noexcept {
        if (!valid || !owner_epoch || (clocked_ && (now < last_ || now-last_ > 150)) ||
            (clocked_ && owner_epoch != owner_)) {
            reset(); return {};
        }
        clocked_ = true; last_ = now; owner_ = owner_epoch;
        const bool started = putdown && !requested_ && !pending_;
        if (started) pending_ = true;
        requested_ = putdown;
        if (pending_ && (physical == SkatePose::riding || physical == SkatePose::air ||
                         physical == SkatePose::bail || physical == SkatePose::grind)) {
            pending_ = false; return {false,true,started};
        }
        return {pending_,false,started};
    }
private:
    std::uint64_t owner_{}, last_{};
    bool pending_{}, requested_{}, clocked_{};
};
}
