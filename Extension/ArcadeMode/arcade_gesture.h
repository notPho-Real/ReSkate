#pragma once
#include "arcade_flip.h"
#include "arcade_push.h"
#include "arcade_board.h"
#include "arcade_classic_input.h"
#include "arcade_manual_input.h"
#include "arcade_grind_input.h"
#include <array>
#include <cstdint>

namespace dingosdk::arcade_mode {
enum class NativeTrick : unsigned { none, ollie, kickflip, heelflip };
struct GesturePhase { unsigned duration_ms; std::int16_t x, y; bool owns_stick; };
struct GestureProfile { std::array<GesturePhase, 3> phases; };
// Distinguish button masking, stick setup and flick from the native packet.
// Restoration must also be visible when the physical packet has not changed.
constexpr std::uint32_t synthetic_packet_mask(unsigned phase, bool masked_buttons) noexcept {
    return phase == 3 ? 0x10000000u : phase == 1 ? 0x40000000u : phase == 2 ? 0x20000000u : masked_buttons ? 0x60000000u : 0u;
}
// XInput Y is up-positive. Flip endpoints are DISCOVERED Deck observations,
// not a claim of repeatability, airborne chaining or stance independence.
constexpr GestureProfile profile(NativeTrick trick) noexcept {
    return {{{{1000, 0, 0, false}, {600, 0, -32768, true},
              {trick == NativeTrick::ollie ? 150u : 250u,
               static_cast<std::int16_t>(trick == NativeTrick::kickflip ? 28000 :
                                         trick == NativeTrick::heelflip ? -28000 : 0),
               static_cast<std::int16_t>(trick == NativeTrick::ollie ? 32767 : 28000), true}}}};
}

class NativeGesture {
public:
    static constexpr std::uint16_t a = 0x1000, x = 0x4000;
    struct Output {
        std::uint16_t buttons;
        std::int16_t right_x, right_y;
        NativeTrick started;
        unsigned phase;
        std::uint8_t requested_id{};
        bool unsupported{};
        bool authored_playback{};
        bool command_busy{};
    };
    void cancel() noexcept {
        active_ = NativeTrick::none;
        previous_ = consumed_ = 0;
        began_ = 0;
        armed_ = false;
    }
    bool busy() const noexcept { return active_ != NativeTrick::none; }
    Output poll(std::uint64_t now, std::uint16_t buttons, std::int16_t lx,
                std::int16_t ly, std::int16_t rx, std::int16_t ry,
                bool game_input, bool may_start) noexcept {
        Output out{buttons, rx, ry, NativeTrick::none, 0};
        const auto pressed = static_cast<std::uint16_t>(buttons & (a | x));
        const auto edges = pressed & ~previous_;
        previous_ = pressed;
        if (!game_input) {
            active_ = NativeTrick::none; consumed_ = 0; armed_ = false;
            return out;
        }
        out.buttons &= ~15;
        // A held command across a menu/focus boundary must be released first.
        if (!armed_) { if (!pressed) armed_ = true; return out; }
        consumed_ &= pressed;
        const bool manual = rx < -8689 || rx > 8689 || ry < -8689 || ry > 8689;
        if (manual) active_ = NativeTrick::none;
        if (active_ == NativeTrick::none && !pressed) consumed_ = 0;
        if (edges && may_start && !manual && active_ == NativeTrick::none) {
            NativeTrick selected = NativeTrick::none;
            std::uint16_t command = 0;
            if (edges & a) { selected = NativeTrick::ollie; command = a; }
            else if (edges & x) {
                auto flip = flip_direction(buttons, lx, ly);
                // Neutral X is the basic kickflip. Opposing/unsupported
                // directions never silently become a different trick.
                if (!(buttons & 15) && lx >= -13000 && lx <= 13000 && ly >= -13000 && ly <= 13000)
                    flip = FlipTrick::kickflip;
                if (flip == FlipTrick::kickflip) selected = NativeTrick::kickflip;
                if (flip == FlipTrick::heelflip) selected = NativeTrick::heelflip;
                command = x;
            }
            if (selected != NativeTrick::none) {
                active_ = out.started = selected; consumed_ |= command; began_ = now;
            }
        }
        out.buttons &= ~consumed_;
        if (active_ == NativeTrick::none) return out;
        if (now < began_) { active_ = NativeTrick::none; return out; }
        auto elapsed = now - began_;
        const auto recipe = profile(active_);
        for (unsigned i = 0; i < recipe.phases.size(); ++i) {
            const auto& phase = recipe.phases[i];
            if (elapsed < phase.duration_ms) {
                if (phase.owns_stick) { out.right_x = phase.x; out.right_y = phase.y; out.phase = i; }
                return out;
            }
            elapsed -= phase.duration_ms;
        }
        active_ = NativeTrick::none;
        if (!pressed) consumed_ = 0;
        return out;
    }
private:
    NativeTrick active_{NativeTrick::none};
    std::uint16_t previous_{}, consumed_{};
    std::uint64_t began_{};
    bool armed_{true};
};

// Own only synthetic state. Every poll starts with the caller's physical input.
class ArcadeInput {
public:
    void set_playback_schedule(AuthoredPlaybackSchedule schedule) noexcept {
        for (auto& gesture : classic_) gesture.set_schedule(schedule);
    }
    void enable(bool value) noexcept {
        if (enabled_ == value) return;
        enabled_ = value;
        cancel_commands();
    }
    void cancel_commands() noexcept {
        for (auto& gesture : gestures_) gesture.cancel();
        for (auto& gesture : classic_) gesture.cancel();
        for (auto& manual : manuals_) manual.cancel();
        for (auto& push : pushes_) push.reset();
        for (auto& chord : chords_) chord.reset();
        for (auto& grind : grinds_) grind.reset();
    }
    void cancel_manuals() noexcept {
        for (auto& manual : manuals_) manual.cancel();
    }
    NativeGesture::Output poll(unsigned user, std::uint64_t now, std::uint16_t buttons,
            std::int16_t lx, std::int16_t ly, std::int16_t rx, std::int16_t ry,
            bool game_input, bool may_start, bool may_push = false, float speed = 0,
            bool triggers_held = false, bool may_toggle_board = false,
            bool authored = false, std::uint64_t tick = 0, bool nollie = false,
            std::span<const PreparedGesture> catalog = {}, bool mirror_x = false,
            bool manual_safe = false, SkatePose observed = SkatePose::unknown,
            bool /* grab experiment selector; air handoff is now shared */ = false, bool manual_prepare_only = false,
            bool offboard = false, bool action_busy = false) noexcept {
        if (!enabled_ || user >= gestures_.size())
            return {buttons, rx, ry, NativeTrick::none, 0};
        NativeGesture::Output out{};
        bool busy{};
        if (game_input && offboard) {
            // A on foot never enters the authored Ollie/flip decoder. Preserve
            // native face/d-pad actions and release every on-board stick owner.
            gestures_[user].cancel(); classic_[user].cancel();
            manuals_[user].cancel(); grinds_[user].reset(); pushes_[user].reset();
            out = {buttons, rx, ry, NativeTrick::none, 0};
        } else if (authored) {
            // A completed native flick's neutral tail must not reserve the
            // airborne window from subsequent X/B or landing-manual commands.
            if (game_input)
                classic_[user].finish_neutral_tail();
            if (game_input && (observed == SkatePose::air || observed == SkatePose::grind) &&
                (buttons & GrindInput::y_button)) classic_[user].finish_neutral_tail();
            const bool classic_game_input = game_input && observed != SkatePose::bail &&
                (observed != SkatePose::unknown || may_start);
            const auto classic = classic_[user].poll(now, tick, buttons, lx, ly, classic_game_input, may_start, nollie, catalog, mirror_x,
                observed == SkatePose::grind ? GestureContext::from_grind : GestureContext::ground,
                observed == SkatePose::air, action_busy && observed == SkatePose::air);
            out = {classic.buttons, classic.owns_stick ? classic.right_x : static_cast<short>(game_input ? 0 : rx),
                classic.owns_stick ? classic.right_y : static_cast<short>(game_input ? 0 : ry), NativeTrick::none,
                classic.owns_stick ? 1u : 0u, classic.requested_id, classic.unsupported};
            busy = classic_[user].busy();
            out.authored_playback = classic.owns_stick;
            const auto manual = manuals_[user].poll(now, buttons, lx, ly, game_input,
                manual_safe, observed, busy || triggers_held || (buttons & 0xf000u), manual_prepare_only);
            if (manual.owns_stick) {
                out.right_x = 0; out.right_y = manual.y; out.phase = 1;
                busy = true;
            }
            const auto grind = grinds_[user].poll(buttons, lx, ly, game_input, observed, busy || triggers_held);
            if (game_input) out.buttons &= ~GrindInput::y_button;
            out.unsupported = out.unsupported || grind.unsupported;
            if (grind.owns_stick) {
                out.right_x = grind.x; out.right_y = grind.y; out.phase = 1; busy = true;
            }
        } else {
            out = gestures_[user].poll(now, buttons, lx, ly, rx, ry, game_input, may_start);
            busy = gestures_[user].busy();
        }
        const auto board = chords_[user].poll(now, out.buttons, game_input,
            may_toggle_board && !busy && !triggers_held && !(buttons & ~BoardChord::pair));
        out.buttons = board.buttons;
        if (board.pulse) out.phase = 3;
        const bool conflict = buttons || triggers_held || busy ||
            (!authored && (rx < -8689 || rx > 8689 || ry < -8689 || ry > 8689));
        if (pushes_[user].poll(now, lx, ly, speed, game_input, may_push && !conflict))
            out.buttons |= NativeGesture::x; // Internal native push; physical X remains a flip request.
        out.command_busy = busy || board.pulse;
        return out;
    }
private:
    bool enabled_{};
    std::array<NativeGesture, 4> gestures_{};
    std::array<ClassicGesture, 4> classic_{};
    std::array<ManualGesture, 4> manuals_{};
    std::array<AutomaticPush, 4> pushes_{};
    std::array<BoardChord, 4> chords_{};
    std::array<GrindInput, 4> grinds_{};
};
}
