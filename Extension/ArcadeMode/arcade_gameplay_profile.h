#pragma once
#include <algorithm>
#include <cstdint>
#include "arcade_launch_descriptor.h"

namespace dingosdk::arcade_mode {
enum class GameplayStats : std::uint8_t { standard = 5, maxed = 10 };
enum class GameplayDifficulty : std::uint8_t { easy, medium, hard };
enum class PhysicalBalanceKind : std::uint8_t { manual, grind, lip };
struct PhysicalBalanceParameters {
    float cheese{}, cheese_frames{}, gravity{}, instability_rate{}, instability_base{};
    float minimum_speed{}, random_speed{}, repeat_minimum{}, repeat_multiplier{}, lean_repeat_multiplier{};
    float correction_acceleration{}, endpoint{}, same_rail_time{}, new_rail_subtraction{};
};
// Configuration only. No menu, scoring or Special-state engine is installed.
// The native caller must supply established stance/modifier observations.
struct GameplayProfile {
    GameplayStats stats{GameplayStats::maxed};
    GameplayDifficulty difficulty{GameplayDifficulty::medium};
    bool switched{}, special{};
    static constexpr float metres_per_inch = .0254f;
    static constexpr float balance_display_scale = 4096.0f;
    constexpr float effective_stat() const noexcept {
        return static_cast<float>(stats == GameplayStats::standard ? 5 : 10) + (special ? 3.0f : 0.0f);
    }
    constexpr float standard_switch() const noexcept {
        return switched ? std::clamp(.9f + .1f * effective_stat() / 10, 0.0f, 1.0f) : 1;
    }
    constexpr float kick_switch() const noexcept {
        return switched ? std::clamp(.8f + .2f * effective_stat() / 10, 0.0f, 1.0f) : 1;
    }
    constexpr float value(float low, float high, float stance_factor = 1) const noexcept {
        return (low + (high - low) * effective_stat() / 10) * stance_factor;
    }
    constexpr float kick_speed(bool crouched) const noexcept {
        return std::min(900.0f, crouched ? value(532, 675, kick_switch()) : value(394, 496, kick_switch())) * metres_per_inch;
    }
    constexpr float kick_acceleration(bool crouched) const noexcept {
        return (crouched ? value(1057, 1200, standard_switch()) : value(629, 700, standard_switch())) * metres_per_inch;
    }
    constexpr float soft_speed() const noexcept { return value(757, 900) * metres_per_inch; }
    constexpr float hard_speed() const noexcept { return value(957, 1100) * metres_per_inch; }
    constexpr float angular_speed() const noexcept { return value(6.85f, 7.75f, standard_switch()); }
    constexpr float flip_base_speed() const noexcept { return value(1.16f, 1.3f, standard_switch()); }
    constexpr float flip_speed(float authored, float cap = 1.3f) const noexcept {
        return std::min(authored * flip_base_speed(), cap);
    }
    constexpr float jump_speed(unsigned charge_ms, bool vert = false, bool boneless = false) const noexcept {
        const float low = vert ? (boneless ? 200.0f : 100.0f) : (boneless ? 400.0f : 350.0f);
        const float high = vert ? (boneless ? value(264, 325, standard_switch()) : value(275, 275, standard_switch())) :
            boneless ? value(489, 525, standard_switch()) : value(414, 450, standard_switch());
        return (low * standard_switch() + (high - low * standard_switch()) *
            static_cast<float>(std::min(charge_ms, 200u)) / 200) * metres_per_inch;
    }
    float launch_speed(unsigned charge_ms, LaunchDescriptor launch) const noexcept {
        if(!launch.known())return 0;
        if(launch.context==LaunchContext::explicit_speed)return launch.speed_mps;
        // Give the ordinary flat ollie more airtime without changing other launch families.
        const float speed = jump_speed(charge_ms,launch.context==LaunchContext::vertical,launch.family==LaunchFamily::extended);
        return launch.ordinary_writer() ? speed * 1.25f : speed;
    }
    constexpr PhysicalBalanceParameters balance(PhysicalBalanceKind kind) const noexcept {
        const auto s = standard_switch();
        const bool manual = kind == PhysicalBalanceKind::manual;
        const bool grind = kind == PhysicalBalanceKind::grind;
        const auto gravity_diff = difficulty == GameplayDifficulty::easy ? .001f : 1.0f;
        const auto correction_diff = difficulty == GameplayDifficulty::easy ? .75f : 1.0f;
        const auto manual_diff = difficulty == GameplayDifficulty::easy ? .5f :
            difficulty == GameplayDifficulty::hard ? 2.0f : 1.0f;
        return {
            manual ? value(700, 700, s) : grind ? value(2500, 2500, s) : value(3000, 1000, s),
            manual ? value(100, 100, s) : grind ? value(30, 30, s) : value(180, 180, s),
            value(.02f, .02f, s) * gravity_diff,
            manual ? value(.099f, .07f, s) * manual_diff : grind ? value(.104f, .09f, s) : value(.5f, .2f, s),
            value(1, 1, s), manual ? value(5, 5, s) : grind ? value(5, 5, s) : value(10, 10, s),
            manual ? value(20, 20, s) : grind ? std::max(6.0f, value(7.07f, 6, s)) : value(20, 20, s),
            value(1, 1, s), manual ? value(.25f, .25f, s) : grind ? value(.31f, .1f, s) : value(1, 1, s),
            manual ? value(.8f, .8f, s) : grind ? value(.607f, .5f, s) : value(1, 1, s),
            value(10, 10, s) * correction_diff, value(4000, 4000, s),
            grind ? value(2, 2, s) : 0, grind ? value(-.286f, 0, s) : 0
        };
    }
};
}
