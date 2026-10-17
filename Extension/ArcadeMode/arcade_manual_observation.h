#pragma once
#include "arcade_native_events.h"

namespace dingosdk::arcade_mode {
struct ManualObservationContext {
    std::uint64_t now{}, generation{};
    bool enabled{}, valid{}, activity_fresh{}, owner_changed{}, gap{};
    SkatePose contact{SkatePose::unknown};
};
inline SkatePose manual_contact_pose(const NativeComboObservation& native, bool pushing) noexcept {
    if (!native.valid || native.bailed || pushing || native.state != 100 || native.mode != 1)
        return SkatePose::unknown;
    if (native.contacts == 0x4000u) return SkatePose::manual;
    if (native.contacts == 0x8000u) return SkatePose::nose_manual;
    return native.contacts == 0xc000u ? SkatePose::riding : SkatePose::unknown;
}
struct ManualObservationResult {
    SkatePose pose{SkatePose::unknown};
    bool confirmed{}, admitted{};
};

// Contact plus generic score activity is only a candidate. Native history must
// independently name the same manual before it can grant combo balance/link.
// Control preparation must continue to use the raw candidate, not this result.
class ManualObservation {
public:
    void reset() noexcept {
        primed_ = confirmed_ = false;
        candidate_ = SkatePose::unknown;
        // Preserve consumed label identity across toggles/owner changes. A
        // persistent history label or replay can never rearm a fresh section.
    }
    void reset(const ScoreLabelOccurrence& label) noexcept {
        consume(label); reset();
    }
    ManualObservationResult poll(SkatePose raw, const ScoreLabelOccurrence& label,
                                 const ManualObservationContext& context) noexcept {
        // A fresh native label admits a section once. Thereafter the current
        // owner's physical wheel contact maintains it, independently of score
        // polling. A short two-wheel contact gap must not blink the meter and
        // leave a still-held native manual without its balance lifecycle.
        const bool same_context = primed_ && context.enabled && context.valid && context.generation &&
            context.generation == generation_ && !context.owner_changed && !context.gap &&
            context.now >= last_at_ && context.now - last_at_ <= ScoreLabelEdges::freshness_ms;
        if (same_context && confirmed_ && (context.contact == candidate_ ||
                (context.contact == SkatePose::riding && context.now >= contact_at_ && context.now - contact_at_ < 100))) {
            last_at_ = context.now;
            if (context.contact == candidate_) contact_at_ = context.now;
            consume(label);
            return {candidate_, true, false};
        }
        const bool manual = raw == SkatePose::manual || raw == SkatePose::nose_manual;
        const auto unconfirmed = [&] {
            return ManualObservationResult{manual ? SkatePose::riding : raw};
        };
        if (!context.enabled || !context.valid || !context.activity_fresh || !context.generation ||
            context.owner_changed || context.gap) {
            consume(label); reset(); return unconfirmed();
        }
        if (!primed_ || context.generation != generation_ || context.now < last_at_ ||
            context.now - last_at_ > ScoreLabelEdges::freshness_ms) {
            reset(); primed_ = true; generation_ = context.generation; last_at_ = context.now;
            candidate_ = raw; candidate_at_ = context.now;
            // A new context supplies a baseline, never an attributable edge.
            consume(label); return unconfirmed();
        }
        last_at_ = context.now;
        if (raw != candidate_) {
            candidate_ = raw; candidate_at_ = context.now; confirmed_ = false;
        }
        const bool repeated = label.sequence == sequence_;
        const bool reordered = label.sequence < sequence_;
        const bool skipped = label.sequence > sequence_ && label.sequence - sequence_ > 1;
        const bool conflicting = repeated && label.labels && !same_edge(label, last_edge_);
        const bool edge = label.labels && !repeated && !reordered && !skipped;
        if (reordered || skipped || conflicting || !label.valid ||
            label.uncertainty != ScoreLabelUncertainty::none) confirmed_ = false;
        consume(label);
        if (!manual) return unconfirmed();
        const auto expected = score_label_bit(raw == SkatePose::manual ? 1 : 2);
        const bool fresh = label.valid && label.uncertainty == ScoreLabelUncertainty::none &&
            label.sequence && label.labels == expected && label.observed_at >= candidate_at_ &&
            label.sampled_at >= label.observed_at && context.now >= label.sampled_at &&
            context.now - label.observed_at <= ScoreLabelEdges::freshness_ms;
        const bool admitted = edge && fresh && !confirmed_;
        if (admitted) { confirmed_ = true; contact_at_ = context.now; }
        return {confirmed_ ? raw : SkatePose::riding, confirmed_, admitted};
    }
private:
    static bool same_edge(const ScoreLabelOccurrence& a, const ScoreLabelOccurrence& b) noexcept {
        return a.valid == b.valid && a.sequence == b.sequence && a.labels == b.labels &&
            a.observed_at == b.observed_at && a.sampled_at == b.sampled_at && a.uncertainty == b.uncertainty;
    }
    void consume(const ScoreLabelOccurrence& label) noexcept {
        if (label.sequence > sequence_) {
            sequence_ = label.sequence;
            last_edge_ = label;
        }
    }
    ScoreLabelOccurrence last_edge_{};
    SkatePose candidate_{SkatePose::unknown};
    std::uint64_t sequence_{}, generation_{}, last_at_{}, candidate_at_{}, contact_at_{};
    bool primed_{}, confirmed_{};
};
}
