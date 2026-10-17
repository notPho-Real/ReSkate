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
        pending_flip_ = air_action_ = was_airborne_ = false; context_ = GestureContext::ground;
    }
    bool busy() const noexcept { return playback_.busy() || pending_flip_; }
    bool finish_neutral_tail() noexcept { return playback_.finish_neutral_tail(); }
    void set_schedule(AuthoredPlaybackSchedule schedule) noexcept {
        if (playback_.schedule() == schedule) return;
        cancel();
        playback_.set_schedule(schedule);
    }
    Output poll(std::uint64_t now, std::uint64_t tick, std::uint16_t buttons,
            std::int16_t lx, std::int16_t ly, bool game_input, bool may_start, bool nollie,
            std::span<const PreparedGesture> catalog, bool mirror_x = false,
            GestureContext context = GestureContext::ground, bool airborne = false,
            bool other_owner = false, bool flip_ready = true) noexcept {
        Output out{buttons};
        if (!game_input) { cancel(); return out; }
        // Directional trick modifiers must not open native gameplay menus.
        out.buttons &= ~(a | x | 15);
        if (clocked_ && (now < last_now_ || tick < last_tick_)) { cancel(); return out; }
        clocked_ = true; last_now_ = now; last_tick_ = tick;
        const auto held = static_cast<std::uint16_t>(buttons & (a | x));
        const auto edges = static_cast<std::uint16_t>(held & ~previous_);
        const bool release_a = (previous_ & a) && !(held & a);
        previous_ = held;
        if (was_airborne_ && !airborne) {
            context_ = GestureContext::ground; count_ = 0; pending_flip_ = false;
        }
        was_airborne_ = airborne;
        if (air_action_ && !airborne) { playback_.cancel(); air_action_ = false; }
        if (other_owner) {
            playback_.cancel(); holding_ = false; count_ = 0; pending_flip_ = air_action_ = false;
            armed_ = held == 0;
            return out;
        }
        if (!armed_) { if (!held) armed_ = true; return out; }
        // A newer grab/grind intent supersedes a waiting flip. A request only
        // lives in this observed air window; landing and loss of context clear it.
        if (buttons & 0xa000u) pending_flip_ = false;
        if ((edges & a) && may_start) playback_.finish_neutral_tail();
        if ((edges & a) && may_start && !playback_.busy()) {
            context_ = context;
            holding_ = begin(nollie ? 16 : 1, nollie, true, tick, catalog, out, mirror_x, context_);
            count_ = 0; air_action_ = false;
        }
        if (release_a && holding_) {
            playback_.release(tick); holding_ = false;
        }
        if ((edges & x) && airborne) {
            playback_.finish_neutral_tail();
            const auto direction = classic_direction(buttons, lx, ly);
            if (!playback_.busy()) {
                count_ = count_ && direction == direction_ && nollie == nollie_ && now - tap_at_ <= 180
                    ? std::min(count_ + 1, 3u) : 1;
                direction_ = direction; nollie_ = nollie; tap_at_ = now;
                const auto id = native_flip_id(select_named_trick(TrickFamily::flip, direction, count_), nollie);
                const auto air_context = context == GestureContext::ground ? context_ : context;
                if (flip_ready) {
                    pending_flip_ = false;
                    air_action_ = begin(id, nollie, false, tick, catalog, out, mirror_x, air_context, true);
                } else {
                    pending_id_ = id; pending_nollie_ = nollie; pending_mirror_ = mirror_x;
                    pending_context_ = air_context; pending_flip_ = id != 0;
                    if (!id) out.unsupported = true;
                }
            }
        }
        if (pending_flip_ && airborne && flip_ready && !playback_.busy()) {
            pending_flip_ = false;
            air_action_ = begin(pending_id_, pending_nollie_, false, tick, catalog, out,
                pending_mirror_, pending_context_, true);
        }
        if (holding_ && !(held & a)) {
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
               std::span<const PreparedGesture> catalog, Output& out, bool mirror_x, GestureContext context,
               bool airborne = false) noexcept {
        if (id) for (const auto& gesture : catalog) {
            if (gesture.native_id != id || gesture.nollie != nollie || gesture.context != context) continue;
            if (playback_.start(gesture.steps, tick, hold, mirror_x, airborne)) { out.requested_id = id; return true; }
        }
        out.unsupported = true;
        return false;
    }
    AuthoredPlayback playback_;
    std::uint16_t previous_{};
    bool armed_{true}, holding_{}, nollie_{}, clocked_{}, air_action_{}, was_airborne_{};
    bool pending_flip_{}, pending_nollie_{}, pending_mirror_{};
    std::uint8_t pending_id_{};
    unsigned count_{};
    std::uint64_t tap_at_{};
    std::uint64_t last_now_{}, last_tick_{};
    TrickDirection direction_{TrickDirection::none};
    GestureContext context_{GestureContext::ground};
    GestureContext pending_context_{GestureContext::ground};
};
}
