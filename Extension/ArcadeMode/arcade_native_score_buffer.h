#pragma once
#include "arcade_native_score_schema.h"
#include <atomic>
#include <cstddef>
#include <span>

namespace dingosdk::arcade_mode::native_score_detail {
struct Event {
    std::uint64_t serial{}, sampled_at{}, epoch{};
    Route route{};
    Payload payload{};
    // Serial is our capture order, not a native trick/line identity. Routes can
    // duplicate a native line; no owner or deduplication claim is implied.
    bool owner_verified{};
    // Optional scalar observations are separate from the monetary payload and
    // supply no owner, occurrence, action-slot, or raw-point proof.
    SequenceDiagnostics sequence{};
};
template<std::size_t Capacity> class Buffer {
    static_assert(Capacity > 0);
    std::atomic_flag busy_ = ATOMIC_FLAG_INIT;
    std::array<Event, Capacity> rows_{};
    std::size_t head_{}, size_{};
    std::uint64_t serial_{};
    std::atomic<std::uint64_t> dropped_{};
public:
    bool push(Event row) noexcept {
        if (busy_.test_and_set(std::memory_order_acquire)) {
            dropped_.fetch_add(1, std::memory_order_relaxed); return false;
        }
        const bool accepted = size_ < Capacity;
        if (accepted) {
            row.serial = ++serial_;
            row.owner_verified = false;
            rows_[(head_ + size_++) % Capacity] = row;
        } else dropped_.fetch_add(1, std::memory_order_relaxed);
        busy_.clear(std::memory_order_release);
        return accepted;
    }
    std::size_t take(std::span<Event> out) noexcept {
        if (busy_.test_and_set(std::memory_order_acquire)) return 0;
        std::size_t n{};
        while (n < out.size() && size_) {
            out[n++] = rows_[head_]; head_ = (head_ + 1) % Capacity; --size_;
        }
        busy_.clear(std::memory_order_release);
        return n;
    }
    std::uint64_t dropped() const noexcept { return dropped_.load(std::memory_order_relaxed); }
};
}
