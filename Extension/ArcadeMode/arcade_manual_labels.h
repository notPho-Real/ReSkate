#pragma once
#include "arcade_manual_history.h"
#include "arcade_score_event.h"
#include <limits>

namespace dingosdk::arcade_mode {
// HIL51: a second Manual adds a distinct presenter while the label union stays
// unchanged. This is corroborating pose evidence only: native contact is still
// required by ManualObservation. Never use it as a monetary or native event ID.
class ManualHistoryLabels {
public:
    void invalidate(ScoreLabelUncertainty why = ScoreLabelUncertainty::unavailable) noexcept {
        primed_ = false; pending_ = 0; uncertainty_ = why;
    }
    void observe(const ManualHistoryDiagnostic& d, std::uint64_t now) noexcept {
        if (!d.available || d.rows > ManualHistory::capacity || d.manual_added > d.manual_distinct ||
            d.nose_added > d.nose_distinct || d.duplicates || d.relabels || d.manual_removed || d.nose_removed ||
            d.rows != d.distinct || d.manual_rows != d.manual_distinct || d.nose_rows != d.nose_distinct) {
            invalidate(); return;
        }
        if (!primed_ || !d.comparable || d.status != ManualHistoryStatus::complete ||
            now < sampled_ || now - sampled_ > ScoreLabelEdges::freshness_ms) {
            const auto why = primed_ && now < sampled_ ? ScoreLabelUncertainty::clock_reset :
                primed_ && now - sampled_ > ScoreLabelEdges::freshness_ms ? ScoreLabelUncertainty::stale :
                ScoreLabelUncertainty::baseline;
            invalidate(why); primed_ = true; sampled_ = now;
            return;
        }
        sampled_ = now;
        if (d.manual_added || d.nose_added) {
            if (sequence_ == std::numeric_limits<std::uint64_t>::max()) { invalidate(); return; }
            ++sequence_;
            if (!pending_) observed_ = now;
            uncertainty_ = pending_ || d.manual_added + d.nose_added != 1 ?
                ScoreLabelUncertainty::coalesced : ScoreLabelUncertainty::none;
            pending_ |= (d.manual_added ? score_label_bit(1) : 0) | (d.nose_added ? score_label_bit(2) : 0);
        } else if (!pending_) uncertainty_ = ScoreLabelUncertainty::none;
    }
    ScoreLabelOccurrence take(std::uint64_t now) noexcept {
        if (primed_ && now < sampled_) invalidate(ScoreLabelUncertainty::clock_reset);
        else if (primed_ && (now - sampled_ > ScoreLabelEdges::freshness_ms ||
                            (pending_ && now - observed_ > ScoreLabelEdges::freshness_ms)))
            invalidate(ScoreLabelUncertainty::stale);
        const ScoreLabelOccurrence result{primed_, pending_, sequence_, observed_, sampled_, uncertainty_};
        pending_ = 0;
        if (primed_) uncertainty_ = ScoreLabelUncertainty::none;
        return result;
    }
private:
    std::uint32_t pending_{};
    std::uint64_t sequence_{}, observed_{}, sampled_{};
    bool primed_{};
    ScoreLabelUncertainty uncertainty_{ScoreLabelUncertainty::unavailable};
};
}
