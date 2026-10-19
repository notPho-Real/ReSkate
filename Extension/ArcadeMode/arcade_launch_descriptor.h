#pragma once
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode {
enum class LaunchFamily : std::uint8_t { unavailable, ordinary, extended };
enum class LaunchContext : std::uint8_t { unavailable, flat, vertical, rail, explicit_speed, inverted };
// Physical launch policy only. It never admits an actor, pose, stage or owner.
struct LaunchDescriptor {
    LaunchFamily family{};
    LaunchContext context{};
    float speed_mps{};
    bool operator==(const LaunchDescriptor&) const = default;
    static constexpr LaunchDescriptor ordinary() noexcept { return {LaunchFamily::ordinary,LaunchContext::flat,0}; }
    bool known() const noexcept {
        return (family==LaunchFamily::ordinary||family==LaunchFamily::extended) &&
            context>=LaunchContext::flat && context<=LaunchContext::inverted && std::isfinite(speed_mps) &&
            (context==LaunchContext::explicit_speed ? speed_mps>0 && speed_mps<1000 : speed_mps==0);
    }
    bool ordinary_writer() const noexcept {
        // Other families/contexts need independently qualified native actors or
        // geometry. A policy descriptor cannot supply that missing admission.
        return known() && family==LaunchFamily::ordinary && context==LaunchContext::flat;
    }
    static constexpr std::uint64_t direction_preload_ms=160, late_ground_ms=333;
    static constexpr bool late_ground_eligible(std::uint64_t now,std::uint64_t departure,bool qualified) noexcept {
        return qualified && now>=departure && now-departure<=late_ground_ms;
    }
};
// The enclosing intent channel's version protects this compound publication.
// Use the same implementation in production and focused transport tests.
struct AtomicLaunchDescriptor {
    std::atomic<std::uint32_t> kind{}, speed{};
    void store(LaunchDescriptor value) noexcept {
        kind.store(static_cast<std::uint32_t>(value.family)|(static_cast<std::uint32_t>(value.context)<<8),std::memory_order_relaxed);
        speed.store(std::bit_cast<std::uint32_t>(value.speed_mps),std::memory_order_relaxed);
    }
    LaunchDescriptor load() const noexcept {
        const auto value=kind.load(std::memory_order_relaxed);
        if(value&0xffff0000u)return {};
        return {static_cast<LaunchFamily>(value&255u),static_cast<LaunchContext>((value>>8)&255u),
            std::bit_cast<float>(speed.load(std::memory_order_relaxed))};
    }
};
}
