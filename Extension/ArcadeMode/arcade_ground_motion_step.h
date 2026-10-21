#pragma once
#include "arcade_native_input_state.h"
#include "arcade_input_policy.h"
#include <cmath>

namespace dingosdk::arcade_mode {
enum class GroundStepStatus : std::uint8_t { unavailable, duplicate, next };
struct GroundStepAdmission {
    GroundStepStatus status{};
    bool baseline{};
    float seconds{};
    explicit operator bool() const noexcept { return status == GroundStepStatus::next; }
};
// A scalar motion budget belongs to one original physics observation. A cached
// dt or a renewed input publication is not another simulation interval. Keep
// this receipt when a controller resets or temporarily loses admission, so the
// same physical step cannot acquire a second budget on recovery or OFF/ON.
class GroundMotionStep {
public:
    GroundStepAdmission consume(const NativeInputFrame& frame, std::uint64_t now,
            bool permitted) noexcept {
        if (!permitted || !frame.owner.valid() || !frame.owner_generation ||
            !frame.contact_generation || !frame.action_generation ||
            (frame.state != 100 && frame.state != 103) || frame.mode > 2 || frame.rail_family || frame.airborne ||
            !InputNativeContext::from_frame(frame).fresh(now) ||
            !std::isfinite(frame.physics_seconds) || frame.physics_seconds < .005f || frame.physics_seconds > .04f) return {};
        const bool same_owner = have_ && frame.owner == previous_.owner;
        if (same_owner) {
            if (frame.physics_step < previous_.physics_step || frame.sampled_at < previous_.sampled_at) return {};
            if (frame.physics_step == previous_.physics_step) {
                const bool same = frame.sampled_at == previous_.sampled_at &&
                    frame.owner_generation == previous_.owner_generation &&
                    frame.contact_generation == previous_.contact_generation &&
                    frame.action_generation == previous_.action_generation &&
                    frame.mode_generation == previous_.mode_generation &&
                    frame.state == previous_.state && frame.mode == previous_.mode &&
                    frame.contacts == previous_.contacts && frame.stance_and_push == previous_.stance_and_push &&
                    frame.physics_seconds == previous_.physics_seconds;
                return {same ? GroundStepStatus::duplicate : GroundStepStatus::unavailable, false, 0};
            }
        }
        const bool baseline = !same_owner || frame.owner_generation != previous_.owner_generation ||
            frame.physics_step - previous_.physics_step != 1 || frame.sampled_at - previous_.sampled_at > 150;
        previous_ = frame; have_ = true;
        return {GroundStepStatus::next, baseline, frame.physics_seconds};
    }
private:
    NativeInputFrame previous_{};
    bool have_{};
};
}
