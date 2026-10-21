#pragma once
#include "arcade_native_score_buffer.h"
#include <array>

namespace dingosdk::arcade_mode {
// These outcomes are evidence classifications, not native reason enum meanings.
enum class ScoreDecision : std::uint8_t {
    valid, duplicate, stale, ambiguous, incomplete, invalid
};
enum class ScoreMeaning : std::uint8_t { unknown, raw_action_points, clean_end, bail, line_start };
struct ScoreProof {
    // Opaque identities from an independently validated adapter. In particular,
    // native occurrence is NOT our capture serial, VM address, or payload value.
    std::uint64_t owner{}, generation{}, combo{}, occurrence{};
    bool local_owner{}, native_identity{}, semantics{};
    ScoreMeaning meaning{ScoreMeaning::unknown};
    // One-based existing model action slot. Mapping a native occurrence to this
    // slot must also be verified; a timestamp coincidence does not supply it.
    unsigned action{};
};
struct ScoreContext {
    std::uint64_t owner{}, generation{}, combo{}, now{};
    // All counters are cumulative within this observer lifetime. A baseline and
    // every subsequent sample are required; missing health is not zero loss.
    std::uint64_t dropped{}, rejected{}, contended{}, stale{};
    bool available{}, health_complete{};
};
struct ScoreReceipt {
    ScoreDecision decision{ScoreDecision::ambiguous};
    ScoreMeaning meaning{ScoreMeaning::unknown};
    std::uint32_t points{};
};

inline ScoreDecision native_score_capture_status(const native_score_detail::Event& event,
    std::uint64_t epoch, std::uint64_t now, bool active) noexcept {
    using native_score_detail::Route;
    if (!active || !epoch || event.epoch != epoch || now < event.sampled_at || now - event.sampled_at > 150)
        return ScoreDecision::stale;
    if (!event.serial || !native_score_detail::score_valid(event.payload.score) ||
        event.route == Route::none || static_cast<unsigned>(event.route) > static_cast<unsigned>(Route::sequence_end) ||
        (event.route == Route::line_start && ((event.payload.score != 0.0) || event.payload.reason || event.payload.success)) ||
        ((event.route == Route::line_end || event.route == Route::stats_line_end) &&
            (event.payload.reason > 8 || event.payload.success)) ||
        (event.route == Route::sequence_end && event.payload.reason))
        return ScoreDecision::invalid;
    // Current captures have neither payload-owner nor canonical occurrence proof.
    // Even setting the diagnostic owner flag alone cannot make this authoritative.
    return ScoreDecision::ambiguous;
}

// One combo's bounded admission journal. The caller establishes a fresh baseline
// before its first event. It never guesses identity from equal point values or
// merges different listeners merely because they arrived close together.
class ScoreBoundary {
public:
    static constexpr std::uint64_t freshness_ms = 150;
    static constexpr std::size_t capacity = 256;
    void begin(const ScoreContext& context, std::uint64_t last_serial) noexcept {
        context_ = context; began_at_ = context.now; serial_ = last_serial; sampled_ = 0;
        size_ = 0; closed_ = have_last_ = false;
        tainted_ = !context.available || !context.health_complete || !context.owner ||
            !context.generation || !context.combo;
    }
    bool tainted() const noexcept { return tainted_; }
    ScoreDecision refresh(const ScoreContext& context) noexcept {
        ScoreDecision decision = ScoreDecision::valid;
        if (!context.available || !context.health_complete || !context.owner || !context.combo)
            decision = ScoreDecision::incomplete;
        else if (context.generation != context_.generation || context.owner != context_.owner ||
                 context.combo != context_.combo || context.now < context_.now)
            decision = ScoreDecision::stale;
        else if (context.dropped != context_.dropped || context.rejected != context_.rejected ||
                 context.contended != context_.contended || context.stale != context_.stale || tainted_)
            decision = ScoreDecision::incomplete;
        if (decision != ScoreDecision::valid) tainted_ = true;
        else context_.now = context.now;
        return decision;
    }
    ScoreReceipt observe(const native_score_detail::Event& event, const ScoreProof& proof,
                         const ScoreContext& context) noexcept {
        const auto reject = [&](ScoreDecision why) {
            tainted_ = true;
            return ScoreReceipt{why};
        };
        if (!context.available || !context.health_complete || !context.owner || !context.combo)
            return reject(ScoreDecision::incomplete);
        if (context.generation != context_.generation || context.owner != context_.owner ||
            context.combo != context_.combo || event.epoch != context.generation ||
            context.now < context_.now || event.sampled_at < began_at_ || event.sampled_at > context.now ||
            context.now - event.sampled_at > freshness_ms)
            return reject(ScoreDecision::stale);
        if (context.dropped != context_.dropped || context.rejected != context_.rejected ||
            context.contended != context_.contended || context.stale != context_.stale)
            return reject(ScoreDecision::incomplete);
        if (native_score_capture_status(event, context.generation, context.now, true) == ScoreDecision::invalid)
            return reject(ScoreDecision::invalid);
        // Replayed transport rows are not native duplicates. An exact repeat of
        // the last capture is harmless only when payload AND authority agree.
        // A previously incomplete stream cannot regain authority via replay.
        if (event.serial <= serial_) {
            if (have_last_ && same_event(event, last_)) {
                if (tainted_) return {ScoreDecision::incomplete};
                if (!same_proof(proof, last_proof_)) return reject(ScoreDecision::ambiguous);
                // Even a duplicate is a new observation of the stream clock.
                context_.now = context.now;
                return {ScoreDecision::duplicate};
            }
            return reject(ScoreDecision::stale);
        }
        if (serial_ == UINT64_MAX || event.serial != serial_ + 1)
            return reject(ScoreDecision::incomplete);
        if (have_last_ && event.sampled_at < sampled_) return reject(ScoreDecision::stale);
        serial_ = event.serial; sampled_ = event.sampled_at;
        last_ = event; last_proof_ = proof; have_last_ = true;
        context_.now = context.now;
        if (tainted_) return {ScoreDecision::incomplete};
        if (!event.owner_verified || !proof.local_owner || !proof.native_identity || !proof.semantics ||
            !proof.occurrence || proof.owner != context.owner || proof.generation != context.generation ||
            proof.combo != context.combo || proof.meaning == ScoreMeaning::unknown ||
            static_cast<unsigned>(proof.meaning) > static_cast<unsigned>(ScoreMeaning::line_start))
            return reject(ScoreDecision::ambiguous);
        const auto route = event.route;
        using native_score_detail::Route;
        if ((proof.meaning == ScoreMeaning::raw_action_points && (route != Route::sequence_end || !event.payload.success)) ||
            ((proof.meaning == ScoreMeaning::clean_end || proof.meaning == ScoreMeaning::bail) &&
                route != Route::line_end && route != Route::stats_line_end) ||
            (proof.meaning == ScoreMeaning::line_start && (route != Route::line_start ||
                event.payload.score != 0 || event.payload.reason || event.payload.success)))
            return reject(ScoreDecision::ambiguous);
        // Aggregate native line/sequence totals are not automatically raw Arcade
        // points. The adapter must independently establish the meaning, including
        // native multiplier/landing factors. Never round a fractional payload.
        if (proof.meaning == ScoreMeaning::raw_action_points &&
            (!proof.action || std::floor(event.payload.score) != event.payload.score))
            return reject(ScoreDecision::ambiguous);
        for (std::size_t i = 0; i < size_; ++i) {
            const auto& prior = journal_[i];
            if (prior.occurrence != proof.occurrence) continue;
            if (prior.meaning == proof.meaning && prior.action == proof.action && prior.payload.score == event.payload.score &&
                prior.payload.reason == event.payload.reason && prior.payload.success == event.payload.success)
                return {ScoreDecision::duplicate}; // includes independently identified duplicate listeners
            return reject(ScoreDecision::ambiguous);
        }
        if (closed_ || size_ == capacity) return reject(ScoreDecision::incomplete);
        journal_[size_++] = {proof.occurrence, proof.meaning, event.payload, proof.action};
        closed_ = proof.meaning == ScoreMeaning::clean_end || proof.meaning == ScoreMeaning::bail;
        return {ScoreDecision::valid, proof.meaning,
            proof.meaning == ScoreMeaning::raw_action_points ? static_cast<std::uint32_t>(event.payload.score) : 0};
    }
private:
    static bool same_proof(const ScoreProof& a, const ScoreProof& b) noexcept {
        return a.owner == b.owner && a.generation == b.generation && a.combo == b.combo &&
            a.occurrence == b.occurrence && a.local_owner == b.local_owner &&
            a.native_identity == b.native_identity && a.semantics == b.semantics &&
            a.meaning == b.meaning && a.action == b.action;
    }
    static bool same_event(const native_score_detail::Event& a, const native_score_detail::Event& b) noexcept {
        return a.serial == b.serial && a.sampled_at == b.sampled_at && a.epoch == b.epoch && a.route == b.route &&
            a.payload.score == b.payload.score && a.payload.reason == b.payload.reason &&
            a.payload.success == b.payload.success && a.sequence == b.sequence && a.owner_verified == b.owner_verified;
    }
    struct Entry {
        std::uint64_t occurrence{};
        ScoreMeaning meaning{};
        native_score_detail::Payload payload{};
        unsigned action{};
    };
    ScoreContext context_{};
    std::array<Entry, capacity> journal_{};
    native_score_detail::Event last_{};
    ScoreProof last_proof_{};
    std::uint64_t serial_{}, sampled_{}, began_at_{};
    std::size_t size_{};
    bool tainted_{true}, closed_{}, have_last_{};
};
}
