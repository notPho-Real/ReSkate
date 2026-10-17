#pragma once
// Adapted from the GPL v3 Style Editor gesture playback in
// worthlessnorms/ReSkate, f6d7e5a929f9e2b046ea6b1192b1fac2487079c7.
#include "arcade_authored_gestures.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <span>
#include <vector>

namespace dingosdk::arcade_mode {
struct AuthoredStickStep {
    unsigned tick{};
    std::int16_t x{}, y{};
};
// Prepare away from the input callback. Authored step offsets use game ticks,
// independent of the number of XInput reads in a frame.
inline std::vector<AuthoredStickStep> compile_authored_flick(const AuthoredGesture& gesture,
                                                           unsigned flick_ms = 24) {
    if (gesture.arcs.empty() || gesture.arcs.size() > 32 || flick_ms > 1000) return {};
    constexpr float pi = 3.14159265358979323846f;
    std::vector<std::array<float, 2>> points;
    points.reserve(gesture.arcs.size());
    for (const auto& arc : gesture.arcs) {
        if (!std::isfinite(arc.start) || !std::isfinite(arc.end) ||
            std::abs(arc.start) > 3600 || std::abs(arc.end) > 3600 ||
            !std::isfinite(arc.minimum) || !std::isfinite(arc.maximum) ||
            arc.minimum < 0 || arc.minimum > 1 || arc.maximum < arc.minimum || arc.maximum > 2)
            return {};
        const auto angle = (arc.start + std::remainder(arc.end - arc.start, 360.0f) / 2 + 180.0f) * pi / 180;
        float magnitude = arc.maximum <= .25f ? 0 : (arc.minimum + arc.maximum) / 2;
        if (points.empty() && arc.maximum >= 1) magnitude = 1;
        points.push_back({std::sin(angle) * magnitude, std::cos(angle) * magnitude});
    }
    const auto axis = [](float value) {
        return static_cast<std::int16_t>(std::clamp(value, -1.0f, 1.0f) * 32767);
    };
    std::vector<AuthoredStickStep> result;
    const auto append = [&](unsigned tick, const std::array<float, 2>& p) {
        result.push_back({tick, axis(p[0]), axis(p[1])});
    };
    append(0, points.front());
    const auto moves = static_cast<unsigned>(std::max<std::size_t>(points.size() - 1, 1));
    const auto total = std::max(moves, (flick_ms * 60 + 500) / 1000);
    unsigned tick = 18;
    for (std::size_t i = 1; i < points.size(); ++i) {
        const auto& a = points[i - 1]; const auto& b = points[i];
        const float turn = std::abs(std::remainder(std::atan2(b[0], b[1]) - std::atan2(a[0], a[1]), 2 * pi)) * 180 / pi;
        const auto least = i > 1 ? static_cast<unsigned>(std::ceil(turn / 90)) : 1u;
        const auto share = std::max(total / moves + (i - 1 < total % moves ? 1u : 0u), least);
        for (unsigned t = 1; t <= share; ++t) {
            const float f = static_cast<float>(t) / static_cast<float>(share);
            append(tick + t - 1, {a[0] + (b[0] - a[0]) * f, a[1] + (b[1] - a[1]) * f});
        }
        tick += share;
    }
    append(tick + 3, {0, 0});
    append(tick + 35, {0, 0});
    return result;
}

enum class AuthoredPlaybackSchedule { delivery_relative, absolute_ticks };

// SessionSticks0.6.1 (GPL-3.0-or-later), core.c build_stroke(rest_first):
// neutral12+18ms, load10ms, hold35ms, travel35ms per point, end60ms.
// Reuse the authored native path, prepared outside the input callback. Quantize
// its continuous stroke to game ticks; repeated polls never shorten a hold.
inline std::vector<AuthoredStickStep> compile_airborne_flick(std::span<const AuthoredStickStep> ground) {
    if (ground.empty() || ground.size() > 128) return {};
    std::vector<AuthoredStickStep> result{{0, 0, 0}, {2, 0, 0}};
    unsigned tick = 2;
    AuthoredStickStep from{};
    bool first = true;
    for (const auto& point : ground) {
        if (!point.x && !point.y) break;
        const unsigned travel = first ? 1 : 2;
        for (unsigned t = 1; t <= travel; ++t) {
            result.push_back({++tick,
                static_cast<std::int16_t>(static_cast<int>(from.x) + (static_cast<int>(point.x) - from.x) * static_cast<int>(t) / static_cast<int>(travel)),
                static_cast<std::int16_t>(static_cast<int>(from.y) + (static_cast<int>(point.y) - from.y) * static_cast<int>(t) / static_cast<int>(travel))});
        }
        if (first) { tick += 2; result.push_back({tick, point.x, point.y}); }
        from = point; first = false;
    }
    if (first || tick + 6 > 180 || result.size() + 2 > 128) return {};
    result.push_back({tick + 4, 0, 0});
    result.push_back({tick + 6, 0, 0});
    return result;
}

class AuthoredPlayback {
public:
    struct Output { bool owns_stick{}; std::int16_t x{}, y{}; };
    void set_schedule(AuthoredPlaybackSchedule schedule) noexcept {
        if (schedule_ == schedule) return;
        cancel();
        schedule_ = schedule;
    }
    AuthoredPlaybackSchedule schedule() const noexcept { return schedule_; }
    bool start(std::span<const AuthoredStickStep> steps, std::uint64_t tick, bool hold_setup,
               bool mirror_x = false, bool airborne = false) noexcept {
        cancel();
        if (airborne && hold_setup) return false;
        if (steps.empty() || steps.size() > steps_.size() || steps.front().tick != 0 ||
            steps.back().tick > 180 || steps.back().x || steps.back().y) return false;
        for (std::size_t i = 1; i < steps.size(); ++i)
            if (steps[i].tick <= steps[i - 1].tick) return false;
        std::copy(steps.begin(), steps.end(), steps_.begin());
        if (mirror_x) for (std::size_t i = 0; i < steps.size(); ++i)
            steps_[i].x = static_cast<std::int16_t>(std::clamp(-static_cast<int>(steps_[i].x), -32768, 32767));
        size_ = steps.size(); began_ = last_ = step_at_ = tick; holding_ = hold_setup;
        for (std::size_t i = 0; i < size_; ++i)
            if (steps_[i].x || steps_[i].y) last_non_neutral_ = i;
        return true;
    }
    void release(std::uint64_t tick) noexcept {
        if (!holding_ || !size_) return;
        if (tick < last_) { cancel(); return; }
        holding_ = false; released_ = true; began_ = last_ = step_at_ = tick;
    }
    Output sample(std::uint64_t tick) noexcept {
        if (!size_) return {};
        if (tick < last_) { cancel(); return {}; }
        last_ = tick;
        if (holding_) return {true, steps_[0].x, steps_[0].y};
        const auto elapsed = tick - began_;
        if (elapsed >= 180) { cancel(); return {}; }
        if (schedule_ == AuthoredPlaybackSchedule::absolute_ticks) {
            // Match the reference script's absolute game-tick timeline. A late
            // poll observes the latest due step without delaying later arcs. cat pouch
            const auto script_tick = elapsed + (released_ ? 18u : 0u);
            while (index_ + 1 < size_ && steps_[index_ + 1].tick <= script_tick) ++index_;
            if (index_ + 1 == size_) { cancel(); return {}; }
            return {true, steps_[index_].x, steps_[index_].y};
        }
        const auto since_step = tick - step_at_;
        if (index_ >= last_non_neutral_ &&
            since_step >= steps_[size_ - 1].tick - steps_[index_].tick) {
            cancel(); return {};
        }
        if (index_ + 1 < size_) {
            const auto spacing = steps_[index_ + 1].tick - steps_[index_].tick;
            const auto due = released_ ? (spacing > 18 ? spacing - 18 : 0) : spacing;
            if (since_step >= due) {
                // A late input poll must deliver the next arc, never jump over it.
                // Schedule later arcs from this delivery; same-tick reads are stable.
                ++index_; step_at_ = tick; released_ = false;
            }
        }
        if (index_ + 1 == size_) { cancel(); return {}; }
        return {true, steps_[index_].x, steps_[index_].y};
    }
    void cancel() noexcept { size_ = index_ = last_non_neutral_ = 0; holding_ = released_ = false; }
    bool busy() const noexcept { return size_ != 0; }
    bool finish_neutral_tail() noexcept {
        // A completed flick has already delivered neutral before a following
        // airborne action may take ownership. Never truncate an authored arc.
        if (!size_ || holding_ || index_ <= last_non_neutral_) return false;
        cancel();
        return true;
    }
private:
    std::array<AuthoredStickStep, 128> steps_{};
    std::size_t size_{}, index_{}, last_non_neutral_{};
    std::uint64_t began_{}, last_{}, step_at_{};
    bool holding_{}, released_{};
    AuthoredPlaybackSchedule schedule_{AuthoredPlaybackSchedule::delivery_relative};
};
}
