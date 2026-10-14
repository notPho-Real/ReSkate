#pragma once
#include "arcade_authored_playback.h"
#include "arcade_native_mapping.h"

namespace dingosdk::arcade_mode {
struct PreparedGesture {
    std::uint8_t native_id{};
    bool nollie{};
    GestureContext context{};
    std::vector<AuthoredStickStep> steps;
};
inline std::vector<PreparedGesture> prepare_gestures(std::span<const AuthoredGesture> catalog) {
    std::vector<PreparedGesture> result;
    for (const auto& gesture : catalog) {
        auto steps = compile_authored_flick(gesture);
        if (!steps.empty()) result.push_back({gesture.native_id, gesture.nollie, gesture.context, std::move(steps)});
    }
    return result;
}
constexpr TrickDirection classic_direction(std::uint16_t buttons, std::int16_t lx, std::int16_t ly) noexcept {
    const auto pad = buttons & 15;
    if ((pad & 3) == 3 || (pad & 12) == 12) return TrickDirection::invalid;
    const int x = pad ? ((pad & 8 ? 1 : 0) - (pad & 4 ? 1 : 0)) : lx > 13000 ? 1 : lx < -13000 ? -1 : 0;
    const int y = pad ? ((pad & 1 ? 1 : 0) - (pad & 2 ? 1 : 0)) : ly > 13000 ? 1 : ly < -13000 ? -1 : 0;
    if (y > 0) return x < 0 ? TrickDirection::up_left : x > 0 ? TrickDirection::up_right : TrickDirection::up;
    if (y < 0) return x < 0 ? TrickDirection::down_left : x > 0 ? TrickDirection::down_right : TrickDirection::down;
    return x < 0 ? TrickDirection::left : x > 0 ? TrickDirection::right : TrickDirection::none;
}

// Decodes player intent; only the independent native observer can prove a result.
class ClassicGesture {
public:
    static constexpr std::uint16_t a = 0x1000, x = 0x4000;
    struct Output {
        std::uint16_t buttons{};
        bool owns_stick{};
        std::int16_t right_x{}, right_y{};
        std::uint8_t requested_id{};
        bool unsupported{};
    };
    void cancel() noexcept {
        playback_.cancel(); previous_ = 0; count_ = 0; holding_ = false; armed_ = false; clocked_ = false;
    }
    bool busy() const noexcept { return playback_.busy() || count_; }
    Output poll(std::uint64_t now, std::uint64_t tick, std::uint16_t buttons,
            std::int16_t lx, std::int16_t ly, bool game_input, bool may_start, bool nollie,
            std::span<const PreparedGesture> catalog) noexcept {
        Output out{buttons};
        if (!game_input) { cancel(); return out; }
        out.buttons &= ~(a | x);
        if (clocked_ && (now < last_now_ || tick < last_tick_)) { cancel(); return out; }
        clocked_ = true; last_now_ = now; last_tick_ = tick;
        const auto held = static_cast<std::uint16_t>(buttons & (a | x));
        const auto edges = static_cast<std::uint16_t>(held & ~previous_);
        const bool release_a = (previous_ & a) && !(held & a);
        previous_ = held;
        if (!armed_) { if (!held) armed_ = true; return out; }
        if (count_ && now < tap_at_) { cancel(); return out; }
        if ((edges & a) && may_start && !playback_.busy()) {
            holding_ = begin(nollie ? 16 : 1, nollie, true, tick, catalog, out);
        }
        if ((edges & x) && ((may_start && !playback_.busy()) || holding_)) {
            const auto direction = classic_direction(buttons, lx, ly);
            if (!count_) { direction_ = direction; nollie_ = nollie; count_ = 1; }
            else if (direction == direction_ && nollie == nollie_ && now - tap_at_ <= 180)
                count_ = std::min(count_ + 1, 3u);
            else { count_ = 0; out.unsupported = true; }
            tap_at_ = now;
        }
        if (count_ && (count_ == 3 || now - tap_at_ >= 180)) {
            const auto trick = select_named_trick(TrickFamily::flip, direction_, count_);
            const auto id = native_flip_id(trick, nollie_);
            const bool hold = holding_;
            if (may_start || holding_) {
                if (!begin(id, nollie_, hold, tick, catalog, out)) {
                    playback_.cancel(); holding_ = false;
                } else holding_ = hold;
            }
            count_ = 0;
        }
        if (release_a && holding_ && !count_) {
            playback_.release(tick); holding_ = false;
        } else if (holding_ && !(held & a) && !count_) {
            playback_.release(tick); holding_ = false;
        }
        const auto sample = playback_.sample(tick);
        if (!playback_.busy()) holding_ = false;
        out.owns_stick = sample.owns_stick;
        out.right_x = sample.x; out.right_y = sample.y;
        return out;
    }
private:
    bool begin(std::uint8_t id, bool nollie, bool hold, std::uint64_t tick,
               std::span<const PreparedGesture> catalog, Output& out) noexcept {
        if (id) for (const auto& gesture : catalog) {
            if (gesture.native_id != id || gesture.nollie != nollie || gesture.context != GestureContext::ground) continue;
            if (playback_.start(gesture.steps, tick, hold)) { out.requested_id = id; return true; }
        }
        out.unsupported = true;
        return false;
    }
    AuthoredPlayback playback_;
    std::uint16_t previous_{};
    bool armed_{true}, holding_{}, nollie_{}, clocked_{};
    unsigned count_{};
    std::uint64_t tap_at_{};
    std::uint64_t last_now_{}, last_tick_{};
    TrickDirection direction_{TrickDirection::none};
};
}
