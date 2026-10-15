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
// Two runtime producers, one background consumer. Per-slot publication prevents
// a reader draining the queue from making a producer discard an otherwise valid
// row. Reservation retries are bounded; full/contended loss remains explicit.
template<std::size_t N> class GrindQueue {
    static_assert(N >= 2 && std::atomic<std::uint64_t>::is_always_lock_free);
    struct Slot { std::atomic<std::uint64_t> turn{}; GrindRow row{}; };
    std::array<Slot, N> rows_{};
    std::atomic<std::uint64_t> write_{}, dropped_{};
    std::uint64_t read_{}; // Accessed only by the background consumer.
public:
    GrindQueue() noexcept {
        for (std::size_t i = 0; i < N; ++i) rows_[i].turn.store(i, std::memory_order_relaxed);
    }
    bool push(GrindRow row) noexcept {
        auto at = write_.load(std::memory_order_relaxed);
        for (unsigned attempt = 0; attempt < 64; ++attempt) {
            auto& slot = rows_[at % N];
            const auto turn = slot.turn.load(std::memory_order_acquire);
            if (turn < at) break; // Consumer has not released this slot: full.
            if (turn == at && write_.compare_exchange_weak(at, at + 1, std::memory_order_relaxed)) {
                row.seq = at + 1;
                row.dropped = dropped_.load(std::memory_order_relaxed);
                slot.row = row;
                slot.turn.store(at + 1, std::memory_order_release);
                return true;
            }
            at = write_.load(std::memory_order_relaxed);
        }
        ++dropped_;
        return false;
    }
    std::size_t take(std::span<GrindRow> out) noexcept {
        std::size_t n{};
        while (n < out.size()) {
            auto& slot = rows_[read_ % N];
            if (slot.turn.load(std::memory_order_acquire) != read_ + 1) break;
            out[n++] = slot.row;
            slot.turn.store(read_ + N, std::memory_order_release);
            ++read_;
        }
        return n;
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
