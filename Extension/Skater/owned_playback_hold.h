#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
namespace dingosdk::skater_animation {
// Borrowed pauses are short, finite submissions. Ordinary/plugin-owned pause
// remains unchanged; this bound is not a gameplay or native physics constant.
inline bool owned_hold_deadline(std::uint64_t now,std::uint64_t deadline) noexcept {
    return deadline>now && deadline-now<=1000;
}
inline float owned_hold_weight(std::uint64_t now,std::uint64_t deadline,
        std::uint64_t released_at,float blend) noexcept {
    const auto end=released_at?std::min(deadline,released_at):deadline;
    if(now<end)return 1;
    if(!std::isfinite(blend) || blend<=0)return 0;
    return 1-std::clamp(static_cast<float>(now-end)/(1000*blend),0.f,1.f);
}
inline std::optional<std::uint64_t> owned_hold_release(std::uint64_t ticket,std::uint64_t borrowed,
        std::uint64_t current,std::uint64_t deadline,bool held,std::uint64_t released,std::uint64_t now) noexcept {
    if(!ticket || ticket!=borrowed || ticket!=current || !deadline || !held)return {};
    return released?released:now;
}
}
