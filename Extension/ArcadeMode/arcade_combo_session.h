#pragma once
#include "arcade_model.h"

namespace dingosdk::arcade_mode {
// Semantic observations supplied by a verified native observer, never buttons.
enum class SkatePose : std::uint8_t { unknown, riding, air, grind, manual, nose_manual, bail };
inline BalanceKind balance_kind(SkatePose pose) noexcept {
    if (pose == SkatePose::grind) return BalanceKind::grind;
    if (pose == SkatePose::manual || pose == SkatePose::nose_manual) return BalanceKind::manual;
    return BalanceKind::none;
}

// Transition adaptation is separate from dynamics and native actuation. A failed
// balance cannot restart the combo on the next frame of the same native section.
class ComboSession {
public:
    explicit ComboSession(Tuning tuning = {}) noexcept : model_(tuning) {}
    const Model& model() const noexcept { return model_; }
    bool failed_section() const noexcept { return failed_section_; }
    void invalidate() noexcept {
        model_.bail();
        previous_ = SkatePose::unknown;
        failed_section_ = false;
    }
    bool step(SkatePose pose, float dt, float correction, bool observed_revert = false) noexcept {
        if (pose == SkatePose::unknown || pose == SkatePose::bail ||
            !std::isfinite(dt) || dt < 0 || dt > 0.25f) {
            invalidate();
            return false;
        }
        const auto kind = balance_kind(pose);
        const bool failure = model_.tick(dt, correction);
        if (failure) failed_section_ = true;
        if (kind == BalanceKind::none) failed_section_ = false;
        if (pose != previous_) {
            if (kind != BalanceKind::none) {
                if (!failed_section_) model_.begin_balance(kind);
            } else if (pose == SkatePose::air) {
                // An observed takeoff starts/links the lifecycle. Native trick
                // scoring is not yet exposed; do not fabricate awarded points.
                model_.trick(0);
            } else if (pose == SkatePose::riding) {
                model_.end_balance();
                if (previous_ == SkatePose::air || balance_kind(previous_) != BalanceKind::none)
                    model_.landing();
            }
        }
        if (observed_revert) model_.revert();
        previous_ = pose;
        return failure;
    }
private:
    Model model_;
    SkatePose previous_{SkatePose::unknown};
    bool failed_section_{};
};
}
