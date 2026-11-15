#pragma once
#include "native_score_snapshot.h"
#include <algorithm>
#include <optional>
#include <utility>

namespace dingosdk::tricks {
struct NativeScoreProgressValue { std::uint64_t points{}; unsigned actions{}; };
// Native sequence numbers are reused even within a line. Follow the live action
// transition and committed history positions instead. Selection pointers and
// HUD labels never count; landing revisions replace the sequence's points.
class NativeScoreProgress {
    struct Entry {
        NativeScoreSequence sequence;
        std::vector<std::pair<unsigned, std::uint64_t>> actions;
        bool completed{};
    };
    static constexpr std::size_t excluded = 256;
public:
    void reset() { *this = {}; }
    std::optional<NativeScoreProgressValue> observe(const std::optional<NativeScoreSnapshot>& next,
                                                   bool active, std::uint64_t settlement) {
        const bool starting = active && (!active_ || settlement != settlement_);
        if (!active || starting) {
            entries_.clear(); slots_.clear(); open_.reset(); uncertain_ = false;
            if (starting) {
                baseline_ = last_;
                if (baseline_) slots_.resize(baseline_->completed.size(), excluded);
            }
        }
        active_ = active; settlement_ = settlement;
        if (!next || !next->manager || !next->hud || !next->scoring) {
            last_.reset();
            if (active) uncertain_ = true;
            else baseline_.reset();
            return std::nullopt;
        }
        if (!active) { baseline_ = last_ = next; return NativeScoreProgressValue{}; }
        if (!baseline_ || !same_owner(*baseline_, *next)) uncertain_ = true;
        if (!uncertain_) {
            // Establish finalization before reading the next live action: a
            // client poll may skip the empty frame between two sequences.
            revise(next->previous);
            for (const auto& seq : next->previous_line) revise(seq);
            completed(*next);
            current(next->current);
        }
        last_ = next;
        if (uncertain_) return std::nullopt;
        NativeScoreProgressValue result;
        for (const auto& entry : entries_) {
            result.points += entry.sequence.points;
            result.actions += static_cast<unsigned>(entry.actions.size());
        }
        return result;
    }
private:
    static bool same_owner(const NativeScoreSnapshot& a, const NativeScoreSnapshot& b) {
        return a.manager == b.manager && a.hud == b.hud && a.scoring == b.scoring;
    }
    static bool same_start(const NativeScoreSequence& a, const NativeScoreSequence& b) {
        return !a.action_objects.empty() && !b.action_objects.empty() &&
            a.action_objects.front() == b.action_objects.front();
    }
    bool predates_combo(const NativeScoreSequence& seq) const {
        if (same_start(seq, baseline_->current) || same_start(seq, baseline_->previous)) return true;
        for (const auto* list : {&baseline_->completed, &baseline_->previous_line})
            for (const auto& old : *list) if (same_start(seq, old)) return true;
        return false;
    }
    bool valid(const NativeScoreSequence& seq) {
        if (seq.action_objects.size() <= 64 && std::none_of(seq.action_objects.begin(), seq.action_objects.end(),
            [](auto action) { return !action; })) return true;
        uncertain_ = true; return false;
    }
    std::size_t append(const NativeScoreSequence& seq) {
        if (entries_.size() == excluded) { uncertain_ = true; return excluded; }
        entries_.push_back({}); update(entries_.back(), seq, false); return entries_.size() - 1;
    }
    void update(Entry& entry, const NativeScoreSequence& seq, bool done) {
        if (!valid(seq)) return;
        for (unsigned slot = 0; slot < seq.action_objects.size(); ++slot) {
            const auto action = std::make_pair(slot, seq.action_objects[slot]);
            if (std::find(entry.actions.begin(), entry.actions.end(), action) != entry.actions.end()) continue;
            if (entry.actions.size() == 128) { uncertain_ = true; return; }
            entry.actions.push_back(action);
        }
        entry.sequence = seq; entry.completed = done;
    }
    void current(const NativeScoreSequence& seq) {
        if (!valid(seq)) return;
        if (seq.action_objects.empty()) { open_.reset(); return; }
        if (predates_combo(seq)) return;
        if (open_) {
            auto& entry = entries_[*open_];
            if (!entry.completed) { update(entry, seq, false); return; }
            if (same_start(entry.sequence, seq)) return; // finalized live copy
        }
        // The live field can keep showing a record already in native history.
        for (std::size_t i = entries_.size(); i > 0; --i)
            if (entries_[i-1].completed && same_start(entries_[i-1].sequence, seq)) { open_ = i-1; return; }
        const auto i = append(seq);
        if (i != excluded) open_ = i;
    }
    void revise(const NativeScoreSequence& seq) {
        if (!valid(seq) || seq.action_objects.empty() || predates_combo(seq)) return;
        // Historical records cannot introduce sequences. A live-to-final handoff
        // may occur before the sequence is appended to CurrentLine's array.
        for (auto& entry : entries_) if (same_start(entry.sequence, seq)) update(entry, seq, true);
    }
    void completed(const NativeScoreSnapshot& next) {
        if (next.completed.size() > 64) { uncertain_ = true; return; }
        bool restarted = next.completed.size() < slots_.size();
        for (std::size_t i = 0; i < std::min(slots_.size(), next.completed.size()); ++i)
            if (slots_[i] != excluded && !same_start(entries_[slots_[i]].sequence, next.completed[i])) restarted = true;
        if (restarted) {
            // A rollover is supported only when the previous native line
            // retains the records that were bound to the disappearing slots.
            for (const auto slot : slots_) if (slot != excluded &&
                std::none_of(next.previous_line.begin(), next.previous_line.end(), [&](const auto& old) {
                    return same_start(old, entries_[slot].sequence);
                })) { uncertain_ = true; return; }
            slots_.clear();
        }
        for (std::size_t i = 0; i < next.completed.size(); ++i) {
            const auto& seq = next.completed[i];
            if (!valid(seq)) return;
            if (i == slots_.size()) {
                std::size_t found = excluded;
                if (!seq.action_objects.empty() && !predates_combo(seq)) {
                    for (std::size_t j = 0; j < entries_.size(); ++j)
                        if (same_start(entries_[j].sequence, seq) &&
                            std::find(slots_.begin(), slots_.end(), j) == slots_.end()) { found = j; break; }
                    if (found == excluded) found = append(seq);
                }
                slots_.push_back(found);
            }
            if (slots_[i] != excluded) update(entries_[slots_[i]], seq, true);
        }
    }
    std::optional<NativeScoreSnapshot> baseline_, last_;
    std::vector<Entry> entries_;
    std::vector<std::size_t> slots_;
    std::optional<std::size_t> open_;
    std::uint64_t settlement_{};
    bool active_{}, uncertain_{};
};
}
