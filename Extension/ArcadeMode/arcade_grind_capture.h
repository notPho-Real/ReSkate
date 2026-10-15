#pragma once
#include "arcade_grind_observer.h"
#include <atomic>
#include <span>

namespace dingosdk::arcade_mode {
enum class GrindEdge : unsigned { sample, entry, exit, invalidated };
struct GrindRow {
    std::uint64_t seq{}, ms{}, generation{}, duration_ms{}, dropped{};
    GrindEdge edge{};
    bool valid{}, grind_valid{}, skate_valid{}, speed_valid{}, requested_y{}, arcade{};
    unsigned state{}, grind{}, skate{}, user{}, kind{};
    int speed_mm_s{}, x{}, y{};
};
// Nonblocking producer/consumer queue. Accepted rows have ordered sequences;
// cumulative loss makes missing transitions explicit, including contention.
template<std::size_t N> class GrindQueue {
    std::atomic_flag busy_ = ATOMIC_FLAG_INIT;
    std::array<GrindRow, N> rows_{};
    std::size_t head_{}, size_{};
    std::atomic<std::uint64_t> seq_{}, dropped_{};
public:
    bool push(GrindRow row) noexcept {
        if (busy_.test_and_set(std::memory_order_acquire)) { ++dropped_; return false; }
        const bool accepted = size_ < N;
        if (accepted) { row.seq = ++seq_; row.dropped = dropped_.load(); rows_[(head_ + size_++) % N] = row; }
        else ++dropped_;
        busy_.clear(std::memory_order_release); return accepted;
    }
    std::size_t take(std::span<GrindRow> out) noexcept {
        if (busy_.test_and_set(std::memory_order_acquire)) return 0;
        std::size_t n{};
        while (n < out.size() && size_) { out[n++] = rows_[head_]; head_ = (head_ + 1) % N; --size_; }
        busy_.clear(std::memory_order_release); return n;
    }
    std::uint64_t dropped() const noexcept { return dropped_.load(); }
};
class GrindTransitions {
    bool known_{}, grinding_{};
    std::uint64_t generation_{}, began_{}, last_{};
public:
    GrindRow sample(GrindRow row) noexcept {
        const bool discontinuity = !row.valid || (known_ && (row.generation != generation_ ||
            row.ms < last_ || row.ms - last_ > 150));
        const bool active = row.valid && row.state >= 400 && row.state < 500;
        if (discontinuity) {
            row.edge = GrindEdge::invalidated; row.duration_ms = 0;
            known_ = grinding_ = false;
        } else if (active && !grinding_) { row.edge = GrindEdge::entry; began_ = row.ms; }
        else if (!active && grinding_) { row.edge = GrindEdge::exit; row.duration_ms = row.ms - began_; }
        if (!discontinuity && active) row.duration_ms = row.ms - began_;
        // A discontinuous active section is not a confirmed entry. Next sample
        // starts a new observation interval; never bridge the missing evidence.
        if (!discontinuity) { known_ = true; grinding_ = active; }
        generation_ = row.generation; last_ = row.ms;
        return row;
    }
};
}
