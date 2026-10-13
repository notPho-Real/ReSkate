#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Pure, bounded mapping from one physical A edge to the native right-stick
// crouch/release gesture measured on the Deck. No game state is written here.
class OllieGesture {
public:
    static constexpr std::uint16_t a_button = 0x1000;
    static constexpr std::uint64_t wait_ms = 1000, crouch_ms = 600, release_ms = 150;
    struct Output { std::uint16_t buttons; std::int16_t right_y; bool started; unsigned phase; };

    Output poll(std::uint64_t now, std::uint16_t buttons, std::int16_t right_y,
                bool game_input, bool may_start, bool manual_stick = false) noexcept {
        const bool held = (buttons & a_button) != 0;
        const bool edge = held && !previous_a_;
        previous_a_ = held;
        if (!game_input) { active_ = false; consumed_ = false; return {buttons, right_y, false, 0}; }
        bool started = false;
        // Physical stick wins. Never mix a synthetic trick with the player's
        // own right-stick trick or restart while A stays held.
        if (active_ && manual_stick) active_ = false;
        if (edge && may_start && !active_ && !manual_stick) {
            active_ = consumed_ = started = true;
            began_ = now;
        }
        if (!held && !active_) consumed_ = false;
        if (consumed_) buttons &= ~a_button;
        if (!active_) return {buttons, right_y, started, 0};
        if (now < began_ || now - began_ >= wait_ms + crouch_ms + release_ms) {
            active_ = false;
            if (!held) consumed_ = false;
            return {buttons, right_y, started, 0};
        }
        const auto elapsed = now - began_;
        unsigned phase = 0;
        if (elapsed >= wait_ms + crouch_ms) { right_y = 32767; phase = 2; }
        else if (elapsed >= wait_ms) { right_y = -32768; phase = 1; }
        return {buttons, right_y, started, phase};
    }
private:
    bool previous_a_{}, active_{}, consumed_{};
    std::uint64_t began_{};
};
}
