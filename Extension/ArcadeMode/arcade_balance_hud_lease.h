#pragma once
#include <chrono>
#include <cstdint>

namespace dingosdk::arcade_mode {
// The physical producer and presentation use the same GetTickCount64 domain.
// Retain the original sample, never a publication-relative age or deadline.
struct BalanceHudSource {
    std::uint64_t sampled_at_ms{};
    bool valid{};
};
struct BalanceHudLease {
    static constexpr std::uint64_t lifetime_ms = 150;
    static bool fresh(std::chrono::milliseconds age) noexcept {
        return age.count() >= 0 && age.count() <= static_cast<std::int64_t>(lifetime_ms);
    }
    static bool fresh(BalanceHudSource source, std::uint64_t now_ms) noexcept {
        return source.valid && now_ms >= source.sampled_at_ms &&
            now_ms - source.sampled_at_ms <= lifetime_ms;
    }
};
}
