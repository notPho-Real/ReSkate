#pragma once
#include "arcade_score_labels.h"
#include <cstdint>

namespace dingosdk::arcade_mode {
enum class ScoreLabelUncertainty : std::uint8_t {
    none, baseline, unavailable, stale, clock_reset, coalesced
};
struct ScoreLabelOccurrence {
    bool valid{};
    std::uint32_t labels{};
    std::uint64_t sequence{}, observed_at{}, sampled_at{};
    ScoreLabelUncertainty uncertainty{ScoreLabelUncertainty::unavailable};
    bool contains(std::uint32_t id) const noexcept {
        return id && id <= score_labels.size() && (labels & (std::uint32_t{1} << (id - 1)));
    }
};
constexpr std::uint32_t score_label_bit(std::uint32_t id) noexcept {
    return id && id <= score_labels.size() ? std::uint32_t{1} << (id - 1) : 0;
}
inline bool observed_revert_label(const ScoreLabelOccurrence& event, std::uint64_t now) noexcept {
    const bool one_revert = event.labels == score_label_bit(3) ||
        event.labels == score_label_bit(4) || event.labels == score_label_bit(25);
    return event.valid && event.sequence && one_revert && event.uncertainty == ScoreLabelUncertainty::none &&
        event.sampled_at >= event.observed_at && now >= event.sampled_at &&
        now - event.observed_at <= 150;
}

// Tracks the union of sanitized labels across the observed HUD slots. A slot
// move or persistent reread is not another trick. Same-label repetition without
// a sampled disappearance is unknowable; this helper never invents that event.
class ScoreLabelEdges {
public:
    static constexpr std::uint64_t freshness_ms = 150;
    void invalidate(ScoreLabelUncertainty why = ScoreLabelUncertainty::unavailable) noexcept {
        primed_ = false; present_ = pending_ = 0;
        uncertainty_ = why;
    }
    void observe(std::uint32_t labels, bool complete, std::uint64_t now) noexcept {
        constexpr auto allowed = (std::uint32_t{1} << score_labels.size()) - 1;
        if (!complete || (labels & ~allowed)) { invalidate(); return; }
        if (primed_ && now < sampled_) invalidate(ScoreLabelUncertainty::clock_reset);
        else if (primed_ && now - sampled_ > freshness_ms) invalidate(ScoreLabelUncertainty::stale);
        sampled_ = now;
        if (!primed_) {
            primed_ = true; present_ = labels;
            uncertainty_ = ScoreLabelUncertainty::baseline;
            return;
        }
        const auto appeared = labels & ~present_;
        present_ = labels;
        if (appeared) {
            ++sequence_;
            if (!pending_) observed_ = now;
            const bool multiple = (appeared & (appeared - 1)) != 0;
            uncertainty_ = pending_ || multiple ? ScoreLabelUncertainty::coalesced : ScoreLabelUncertainty::none;
            pending_ |= appeared;
        } else if (!pending_) uncertainty_ = ScoreLabelUncertainty::none;
    }
    // Exactly one client-thread consumer drains each pending edge. A fresh
    // persistent label yields a valid empty snapshot, never another occurrence.
    ScoreLabelOccurrence take(std::uint64_t now) noexcept {
        if (primed_ && now < sampled_) invalidate(ScoreLabelUncertainty::clock_reset);
        else if (primed_ && (now - sampled_ > freshness_ms ||
                            (pending_ && now - observed_ > freshness_ms)))
            invalidate(ScoreLabelUncertainty::stale);
        ScoreLabelOccurrence result{primed_, pending_, sequence_, observed_, sampled_, uncertainty_};
        pending_ = 0;
        if (primed_) uncertainty_ = ScoreLabelUncertainty::none;
        return result;
    }
private:
    std::uint32_t present_{}, pending_{};
    std::uint64_t sequence_{}, observed_{}, sampled_{};
    bool primed_{};
    ScoreLabelUncertainty uncertainty_{ScoreLabelUncertainty::unavailable};
};
}
