#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Request the native board action once; only native observations establish its result.
class BoardChord {
public:
    static constexpr std::uint16_t lb = 0x0100, rb = 0x0200, pair = lb | rb, native_y = 0x8000;
    static constexpr std::uint64_t window_ms = 120, pulse_ms = 120;
    struct Output { std::uint16_t buttons; bool pulse; };
    void reset() noexcept { *this = {}; }
    Output poll(std::uint64_t now, std::uint16_t buttons, bool gameplay, bool permitted) noexcept {
        Output out{buttons, false};
        if (!gameplay || (sampled_ && now < previous_time_)) { reset(); return out; }
        sampled_ = true; previous_time_ = now;
        const auto held = static_cast<std::uint16_t>(buttons & pair);
        const auto edges = static_cast<std::uint16_t>(held & ~previous_);
        previous_ = held;
        if (!held) { armed_ = true; waiting_ = latched_ = consumed_ = pulsing_ = false; return out; }
        if (!armed_) return out;
        if (!latched_ && edges) {
            if (!waiting_) { first_ = now; waiting_ = true; }
            if (held == pair) {
                latched_ = true;
                if (now - first_ <= window_ms && permitted && !(buttons & ~pair)) {
                    began_ = now; consumed_ = pulsing_ = true;
                }
            }
        }
        if (waiting_ && now - first_ > window_ms) latched_ = true;
        if (consumed_) out.buttons &= ~pair;
        if (!permitted || now - began_ >= pulse_ms) pulsing_ = false;
        if (pulsing_) { out.buttons |= native_y; out.pulse = true; }
        return out;
    }
private:
    bool armed_{}, waiting_{}, latched_{}, consumed_{}, pulsing_{}, sampled_{};
    std::uint16_t previous_{};
    std::uint64_t first_{}, began_{}, previous_time_{};
};
}
