#pragma once
#include "arcade_model.h"
#include "arcade_score_boundary.h"
#include <optional>

namespace dingosdk::arcade_mode {
// Admission and settlement surround the existing model, not a second scoring
// engine. The production observer currently cannot supply ScoreProof; therefore
// this boundary is dormant until an independently verified adapter exists.
class ScoreSettlement {
public:
    bool begin(Model& model, const ScoreContext& context, std::uint64_t last_serial) noexcept {
        if (output_) return false; // never erase a settlement its consumer has not taken
        if (!model.state().combo_active || model.state().raw_points ||
            (started_ && context.owner == owner_ && context.generation == generation_ && context.combo == combo_)) {
            invalidate(model); return false;
        }
        gate_.begin(context, last_serial);
        owner_ = context.owner; generation_ = context.generation; combo_ = context.combo;
        model_sequence_ = model.last_settlement().sequence;
        confirmation_ = ScoreMeaning::unknown; pending_.reset(); output_.reset(); emitted_ = credited_ = false;
        actions_.fill(false); action_count_ = 0;
        started_ = true; uncertain_ = gate_.tainted();
        return !uncertain_;
    }
    ScoreReceipt observe(Model& model, const native_score_detail::Event& event,
                         const ScoreProof& proof, const ScoreContext& context) noexcept {
        if (!started_) return {};
        const auto receipt = gate_.observe(event, proof, context);
        if (receipt.decision == ScoreDecision::duplicate) return receipt;
        if (receipt.decision != ScoreDecision::valid) { invalidate(model); return receipt; }
        if (uncertain_ || emitted_) return {ScoreDecision::incomplete};
        if (receipt.meaning == ScoreMeaning::raw_action_points) {
            if (!proof.action || proof.action > actions_.size() || proof.action > model.state().multiplier ||
                actions_[proof.action - 1]) {
                invalidate(model); return {ScoreDecision::ambiguous};
            }
            if (!model.credit_points(receipt.points)) {
                invalidate(model); return {ScoreDecision::stale};
            }
            credited_ = true;
            actions_[proof.action - 1] = true; ++action_count_;
        } else if (receipt.meaning == ScoreMeaning::clean_end || receipt.meaning == ScoreMeaning::bail) {
            confirmation_ = receipt.meaning;
            // Clean native finalization cannot bank while Arcade is still
            // actively linking. A verified bail is an actual losing terminal.
            if (receipt.meaning == ScoreMeaning::bail && model.state().combo_active) model.bail();
        }
        poll(model);
        return receipt;
    }
    void poll(Model& model) noexcept {
        if (!started_ || uncertain_ || emitted_) return;
        const auto outcome = model.last_settlement();
        if (outcome.sequence != model_sequence_) {
            if (model_sequence_ == UINT64_MAX || outcome.sequence != model_sequence_ + 1 || pending_) {
                invalidate(model); return;
            }
            model_sequence_ = outcome.sequence;
            pending_ = outcome;
        }
        if (!pending_) return;
        if (model.state().combo_active) { invalidate(model); return; }
        const bool bank = pending_->kind == SettlementKind::bank && confirmation_ == ScoreMeaning::clean_end;
        const bool bail = pending_->kind == SettlementKind::bail && confirmation_ == ScoreMeaning::bail;
        if (confirmation_ != ScoreMeaning::unknown && !bank && !bail) { invalidate(model); return; }
        if (credited_ && action_count_ == pending_->actions && (bank || bail)) {
            output_ = pending_; emitted_ = true;
        }
    }
    // Absence means unverified/pending, never a fabricated zero-point bank.
    std::optional<ModelSettlement> take() noexcept { auto value = output_; output_.reset(); return value; }
    bool unverified() const noexcept { return !started_ || uncertain_ || !credited_ || (pending_ && !emitted_); }
    void invalidate(Model& model) noexcept {
        uncertain_ = true; pending_.reset();
        // A confirmed terminal is immutable once published, even if the menu,
        // owner or next combo changes before its consumer drains the receipt.
        if (!emitted_) output_.reset();
        if (started_ && !emitted_) model.discard_unverified_points();
    }
private:
    ScoreBoundary gate_;
    std::uint64_t owner_{}, generation_{}, combo_{}, model_sequence_{};
    ScoreMeaning confirmation_{ScoreMeaning::unknown};
    std::optional<ModelSettlement> pending_, output_;
    std::array<bool, ScoreBoundary::capacity> actions_{};
    unsigned action_count_{};
    bool started_{}, uncertain_{true}, emitted_{}, credited_{};
};
}
