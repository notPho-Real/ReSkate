#pragma once
#include <chrono>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Presentation retains the physical producer's existing150ms lease; client or
// render polling cannot renew an older native observation.
struct BalanceHudLease {
    static constexpr std::uint64_t lifetime_ms = 150;
    static bool fresh(std::chrono::milliseconds age) noexcept {
        return age.count() >= 0 && age.count() <= static_cast<std::int64_t>(lifetime_ms);
    }
};
}
