#pragma once
#include "arcade_native_score_schema.h"
#include <atomic>

namespace dingosdk::arcade_mode::native_score_detail {
enum class CaptureSkip : std::uint8_t { unbound, expired, changed };
struct CaptureSkipSnapshot {
    std::uint64_t unbound{}, expired{}, changed{};
};
// Counters describe skipped hook entries, not unique native events. In particular,
// unbound nested interpreter entries have not reached duplicate suppression.
inline void count_capture_skip(std::atomic<std::uint64_t>& counter,
                               std::atomic<bool>& overflow) noexcept {
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
    static_assert(std::atomic<bool>::is_always_lock_free);
    if (counter.fetch_add(1, std::memory_order_relaxed) == UINT64_MAX)
        overflow.store(true, std::memory_order_relaxed);
}
class CaptureHealth {
public:
    bool skip(Route route, CaptureSkip reason) noexcept {
        const auto index = static_cast<unsigned>(route);
        if (!index || index > counts_.size()) return false;
        auto& row = counts_[index - 1];
        switch (reason) {
        case CaptureSkip::unbound: count_capture_skip(row.unbound, overflow_); break;
        case CaptureSkip::expired: count_capture_skip(row.expired, overflow_); break;
        case CaptureSkip::changed: count_capture_skip(row.changed, overflow_); break;
        default: return false;
        }
        return true;
    }
    CaptureSkipSnapshot snapshot(Route route) const noexcept {
        const auto index = static_cast<unsigned>(route);
        if (!index || index > counts_.size()) return {};
        const auto& row = counts_[index - 1];
        return {row.unbound.load(std::memory_order_relaxed), row.expired.load(std::memory_order_relaxed),
                row.changed.load(std::memory_order_relaxed)};
    }
    bool overflow() const noexcept { return overflow_.load(std::memory_order_relaxed); }
private:
    struct Counts { std::atomic<std::uint64_t> unbound{}, expired{}, changed{}; };
    std::array<Counts, 4> counts_{};
    std::atomic<bool> overflow_{};
};
}
