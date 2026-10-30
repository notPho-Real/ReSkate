#pragma once
#include <chrono>
#include <atomic>
#include <cstdint>

namespace dingosdk::arcade_mode {
// The physical producer and presentation use the same GetTickCount64 domain.
// Retain the original sample, never a publication-relative age or deadline.
struct BalanceHudSource {
    std::uint64_t sampled_at_ms{};
    bool valid{};
    // Diagnostic-only identity copied with the immutable publication. Zero
    // leaves ordinary HUD behavior unchanged and conveys no native proof.
    std::uint64_t capture{},request{},owner_generation{},physics_step{},publication{};
    // Static runtime episode token: presentation freshness is not gameplay lifetime.
    const std::atomic<std::uint64_t>* episode{};
    std::uint64_t episode_id{};
};
struct BalanceHudLease {
    static constexpr std::uint64_t lifetime_ms = 150;
    static bool fresh(std::chrono::milliseconds age) noexcept {
        return age.count() >= 0 && age.count() <= static_cast<std::int64_t>(lifetime_ms);
    }
    static bool fresh(BalanceHudSource source, std::uint64_t now_ms) noexcept {
        if(source.episode)return source.episode_id && source.episode->load(std::memory_order_acquire)==source.episode_id;
        return source.valid && now_ms >= source.sampled_at_ms &&
            now_ms - source.sampled_at_ms <= lifetime_ms;
    }
};
}
