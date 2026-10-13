#pragma once
#include "arcade_flip.h"
#include <array>
#include <cstdint>

namespace dingosdk::arcade_mode {
enum class NativeTrick : unsigned { none, ollie, kickflip, heelflip };
struct GesturePhase { unsigned duration_ms; std::int16_t x, y; bool owns_stick; };
struct GestureProfile { std::array<GesturePhase, 3> phases; };
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
    };
    Output poll(std::uint64_t now, std::uint16_t buttons, std::int16_t lx,
                std::int16_t ly, std::int16_t rx, std::int16_t ry,
                bool game_input, bool may_start) noexcept {
        Output out{buttons, rx, ry, NativeTrick::none, 0};
        const auto pressed = buttons & (a | x);
        const auto edges = pressed & ~previous_;
        previous_ = pressed;
        if (!game_input) {
            active_ = NativeTrick::none; consumed_ = 0; armed_ = false;
            return out;
        }
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
}
