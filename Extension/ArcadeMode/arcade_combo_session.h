#pragma once
#include "arcade_model.h"
#include "Extension/Skater/native_score_progress.h"
#include "arcade_custom_score.h"
#include "arcade_score_settlement.h"
#include "arcade_manual_physics.h"

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
    explicit ComboSession(Tuning tuning = {}, bool native_scoring = false) noexcept : model_(tuning, 1, native_scoring) {}
    bool observe_native_score(const std::optional<tricks::NativeScoreSnapshot>& snapshot) {
        const auto progress = native_scoring_.observe(snapshot, model_.state().combo_active,
            model_.last_settlement().sequence);
        model_.native_score(progress ? progress->points : 0, progress ? progress->actions : 0);
        return progress.has_value();
    }
    const Model& model() const noexcept { return model_; }
    const CustomTrickScoring& custom_scoring() const noexcept {return custom_scoring_;}
    bool observe_custom(const CustomTrickExecution& e,const NativeInputOwner& owner,
                        std::uint64_t generation,std::uint64_t now) noexcept {
        return custom_scoring_.observe(model_,e,owner,generation,now);
    }
    bool failed_section() const noexcept { return failed_section_; }
    bool manual_putdown() const noexcept { return manual_putdown_; }
    const State& balance_display_state() const noexcept {
        // A protected or pending native bail can leave the physical manual in
        // place. Keep its failed meter at the edge until that native section
        // actually ends; never restart the model or retain synthetic input.
        if (failed_section_ && balance_kind(previous_) == BalanceKind::manual &&
            model_.failed_balance().balance_kind == BalanceKind::manual)
            return model_.failed_balance();
        return model_.state();
    }
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
        custom_scoring_.clear();
        native_scoring_.reset();
        scoring_.invalidate(model_);
        model_.bail();
        previous_ = SkatePose::unknown;
        failed_section_ = false;
        manual_putdown_ = false;
        exit_elapsed_ = 0;
    }
    bool step(SkatePose pose, float dt, float correction, bool observed_revert = false,
              const PhysicalBalanceSample* physical = nullptr, bool pending_manual_revert = false, bool manual_episode = false) noexcept {
        custom_scoring_.synchronize(model_);
        if(!model_.state().combo_active)custom_scoring_.clear();
        if(manual_episode && balance_kind(pose)==BalanceKind::manual && !failed_section_ && !manual_putdown_) {
            model_.begin_balance(BalanceKind::manual);
            model_.manual_transition(observed_revert);
            if(!pending_manual_revert)model_.manual_continue();
            bool failure=false;
            if(physical && physical->valid){
                model_.tick(dt,0,1,BalanceEndpoints::bail_both,false,true);
                failure=model_.apply_physical_balance(physical->position,physical->velocity,physical->exit);
                failed_section_=failure;manual_putdown_=physical->exit==BalanceExit::putdown;
            }
            previous_=pose;scoring_.poll(model_);return failure;
        }
        if(manual_episode && (pose==SkatePose::air || pose==SkatePose::riding) && !failed_section_ && !manual_putdown_) {
            model_.manual_airborne();previous_=pose;scoring_.poll(model_);return false;
        }
        if (pose == SkatePose::unknown ||
            (physical && !physical->valid) ||
            !std::isfinite(dt) || dt < 0 || dt > (physical && physical->valid && balance_kind(pose)==BalanceKind::manual ? 1.0f : 0.25f)) {
            invalidate();
            return false;
        }
        if (pose == SkatePose::bail) {
            model_.bail();
            scoring_.poll(model_);
            previous_ = pose; failed_section_ = manual_putdown_ = false; exit_elapsed_ = 0;
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
        // Basic Manual and Nose Manual swap their physical endpoint outcomes.
        // Rail balance retains its existing two-sided failure behavior.
        const auto endpoints = previous_ == SkatePose::manual ? BalanceEndpoints::positive_bail :
            previous_ == SkatePose::nose_manual ? BalanceEndpoints::negative_bail : BalanceEndpoints::bail_both;
        bool failure = model_.tick(dt, correction, 1, endpoints, physical == nullptr,
            pending_manual_revert && !failed_section_ && !manual_putdown_ &&
            (pose==SkatePose::riding || kind==BalanceKind::manual));
        if (model_.balance_exit() == BalanceExit::putdown) manual_putdown_ = true;
        if (failure) failed_section_ = true;
        if (kind == BalanceKind::none) failed_section_ = manual_putdown_ = false;
        if (observed_revert && !failed_section_ && !manual_putdown_) model_.revert();
        if (pose != previous_) {
            if (kind != BalanceKind::none) {
                if (!failed_section_ && !manual_putdown_) model_.begin_balance(kind);
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
        if (physical && kind == BalanceKind::manual && !failed_section_ && !manual_putdown_) {
            failure |= model_.apply_physical_balance(physical->position, physical->velocity, physical->exit);
            if (failure) failed_section_ = true;
            if (physical->exit == BalanceExit::putdown) manual_putdown_ = true;
        }
        scoring_.poll(model_);
        previous_ = pose;
        return failure;
    }
private:
    Model model_;
    CustomTrickScoring custom_scoring_;
    tricks::NativeScoreProgress native_scoring_;
    ScoreSettlement scoring_;
    SkatePose previous_{SkatePose::unknown};
    bool failed_section_{};
    bool manual_putdown_{};
    float exit_elapsed_{};
};
}
