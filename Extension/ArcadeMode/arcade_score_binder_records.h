#pragma once
#include "arcade_score_binder_diagnostic.h"
#include "arcade_score_capture_health.h"
#include <atomic>
#include <span>

namespace dingosdk::arcade_mode::native_score_detail {
// No native addresses or borrowed payloads survive the callback. The pre-bind
// observer has no completion/lifetime token, so these rows prove no relationship.
struct BinderRecordRow {
    std::uint64_t serial{}, ms{}, epoch{};
    Route route{};
    std::array<BinderRecord, 3> records{};
};
template<std::size_t Capacity> class BinderRecordBuffer {
    static_assert(Capacity > 0 && Capacity <= 128);
    std::atomic_flag busy_ = ATOMIC_FLAG_INIT;
    std::array<BinderRecordRow, Capacity> rows_{};
    std::array<BinderRecordRow, 2> previous_{};
    std::size_t head_{}, size_{};
    std::uint64_t epoch_{}, serial_{};
    std::atomic<std::uint64_t> dropped_{}, duplicates_{}, stale_{};
    std::atomic<bool> overflow_{};
public:
    bool push(std::uint64_t epoch, std::uint64_t ms, const BinderDiagnostic& diagnostic) noexcept {
        if (!epoch || !diagnostic.records_available ||
            (diagnostic.route != Route::line_end && diagnostic.route != Route::sequence_end)) return false;
        if (busy_.test_and_set(std::memory_order_acquire)) {
            count_capture_skip(dropped_, overflow_); return false;
        }
        bool accepted{};
        auto& previous = previous_[diagnostic.route == Route::line_end ? 0 : 1];
        if (epoch < epoch_ || (previous.epoch == epoch && ms < previous.ms)) {
            count_capture_skip(stale_, overflow_);
        } else if (previous.epoch == epoch && previous.records == diagnostic.records) {
            count_capture_skip(duplicates_, overflow_);
        } else if (size_ == Capacity || serial_ == UINT64_MAX) {
            count_capture_skip(dropped_, overflow_);
            if (serial_ == UINT64_MAX) overflow_.store(true, std::memory_order_relaxed);
        } else {
            epoch_ = epoch;
            previous = {++serial_, ms, epoch, diagnostic.route, diagnostic.records};
            rows_[(head_ + size_++) % Capacity] = previous;
            accepted = true;
        }
        if (epoch > epoch_) epoch_ = epoch;
        busy_.clear(std::memory_order_release);
        return accepted;
    }
    std::size_t take(std::span<BinderRecordRow> output) noexcept {
        if (busy_.test_and_set(std::memory_order_acquire)) return 0;
        std::size_t count{};
        while (count < output.size() && size_) {
            output[count++] = rows_[head_]; head_ = (head_ + 1) % Capacity; --size_;
        }
        busy_.clear(std::memory_order_release);
        return count;
    }
    std::uint64_t dropped() const noexcept { return dropped_.load(std::memory_order_relaxed); }
    std::uint64_t duplicates() const noexcept { return duplicates_.load(std::memory_order_relaxed); }
    std::uint64_t stale() const noexcept { return stale_.load(std::memory_order_relaxed); }
    bool overflow() const noexcept { return overflow_.load(std::memory_order_relaxed); }
};
}
