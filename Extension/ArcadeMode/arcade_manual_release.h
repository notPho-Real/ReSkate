#pragma once
#include "arcade_combo_session.h"
#include "arcade_native_input_state.h"
#include <cstdint>

namespace dingosdk::arcade_mode {
// Putdown delivery and observed physical completion are separate. A model
// endpoint cannot establish an exit, and an unknown contact cannot complete it.
class ManualRelease {
public:
    struct Output { bool pending{}, completed{}, started{}; };
    void reset() noexcept { pending_ = requested_ = clocked_ = false; }
    bool pending() const noexcept { return pending_; }
    static bool completed_contact(const NativeInputFrame& f) noexcept {
        return f.valid && ((f.state == 100 && f.mode == 1 && f.contacts == 0xc000u) ||
            f.state == 200 || f.state == 201 || f.state == 300 || f.state == 504 ||
            f.state == 701 || f.rail_family);
    }
    // A buffered native completion keeps its original timestamp. An older
    // completion cannot retire a newly started release on the same owner.
    void retire_observed(std::uint64_t now, const NativeInputFrame& f) noexcept {
        if (pending_ && f.owner.valid() && f.owner_generation == owner_ && f.physics_step &&
            (!began_step_ || f.physics_step > began_step_) && f.sampled_at >= began_ && f.sampled_at <= now && now-f.sampled_at <= 150 &&
            completed_contact(f)) reset();
    }
    Output step(std::uint64_t now, std::uint64_t owner_epoch, bool valid,
                SkatePose physical, bool putdown, std::uint64_t physics_step = 0) noexcept {
        if (!valid || !owner_epoch || (clocked_ && (now < last_ || now-last_ > 150)) ||
            (clocked_ && owner_epoch != owner_)) {
            reset(); return {};
        }
        clocked_ = true; last_ = now; owner_ = owner_epoch;
        const bool started = putdown && !requested_ && !pending_;
        if (started) { pending_ = true; began_ = now; began_step_ = physics_step; }
        requested_ = putdown;
        if (pending_ && (physical == SkatePose::riding || physical == SkatePose::air ||
                         physical == SkatePose::bail || physical == SkatePose::grind)) {
            pending_ = false; return {false,true,started};
        }
        return {pending_,false,started};
    }
private:
    std::uint64_t owner_{}, last_{}, began_{}, began_step_{};
    bool pending_{}, requested_{}, clocked_{};
};
}
