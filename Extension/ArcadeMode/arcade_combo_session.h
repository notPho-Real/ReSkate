#pragma once
#include "arcade_model.h"
#include "arcade_score_settlement.h"

namespace dingosdk::arcade_mode {
// Semantic observations supplied by a verified native observer, never buttons.
enum class SkatePose : std::uint8_t { unknown, riding, air, grind, manual, nose_manual, bail, grind_exit_transition };
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
    bool begin_score_evidence(const ScoreContext& context, std::uint64_t last_serial) noexcept {
        return scoring_.begin(model_, context, last_serial);
    }
    ScoreReceipt observe_score(const native_score_detail::Event& event, const ScoreProof& proof,
                              const ScoreContext& context) noexcept {
        return scoring_.observe(model_, event, proof, context);
    }
    std::optional<ModelSettlement> take_score_settlement() noexcept { return scoring_.take(); }
    ScoreDecision refresh_score_evidence(const ScoreContext& context) noexcept {
        return scoring_.refresh(model_, context);
    }
    bool score_unverified() const noexcept { return scoring_.unverified(model_); }
    void invalidate() noexcept {
        scoring_.invalidate(model_);
        model_.bail();
        previous_ = SkatePose::unknown;
        failed_section_ = false;
        exit_elapsed_ = 0;
    }
    bool step(SkatePose pose, float dt, float correction, bool observed_revert = false) noexcept {
        if (pose == SkatePose::unknown ||
            !std::isfinite(dt) || dt < 0 || dt > 0.25f) {
            invalidate();
            return false;
        }
        if (pose == SkatePose::bail) {
            model_.bail();
            scoring_.poll(model_);
            previous_ = pose; failed_section_ = false; exit_elapsed_ = 0;
            return false;
        }
        // The fixed rail records 403 -> 701 -> 200/201. Preserve fatigue across
        // this short native handoff, without calling it a trick or keeping a HUD
        // active after rail contact ends. cat pouch
        if (pose == SkatePose::grind_exit_transition) {
            if ((previous_ != SkatePose::grind && previous_ != pose) ||
                failed_section_ || !model_.state().combo_active ||
                (exit_elapsed_ += dt) > 0.15f) {
                invalidate();
                return false;
            }
            model_.end_balance();
            model_.tick(dt);
            scoring_.poll(model_);
            previous_ = pose;
            return false;
        }
        exit_elapsed_ = 0;
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
                if (previous_ == SkatePose::air || previous_ == SkatePose::grind_exit_transition ||
                    balance_kind(previous_) != BalanceKind::none)
                    model_.landing();
            }
        }
        if (observed_revert) model_.revert();
        scoring_.poll(model_);
        previous_ = pose;
        return failure;
    }
private:
    Model model_;
    ScoreSettlement scoring_;
    SkatePose previous_{SkatePose::unknown};
    bool failed_section_{};
    float exit_elapsed_{};
};
}
