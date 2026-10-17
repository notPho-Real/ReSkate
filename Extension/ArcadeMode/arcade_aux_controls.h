#pragma once
#include "arcade_classic_input.h"

namespace dingosdk::arcade_mode {
// Semantic B/Y requests only. The caller decides whether a native method is
// enabled; selection alone never owns the game-facing pad or extends a combo.
class AuxiliaryControls {
public:
    struct Request {
        bool ready{}, rejected{};
        TrickFamily family{TrickFamily::grab};
        ArcadeTrick trick{ArcadeTrick::none};
        TrickDirection direction{TrickDirection::none};
        unsigned presses{}, direction_presses{};
    };
    void cancel() noexcept { *this = {}; armed_ = false; }
    Request poll(std::uint64_t now, std::uint16_t buttons, short lx, short ly,
                 bool gameplay_fresh, bool conflicting, RailRelationship rail = RailRelationship::unknown,
                 bool immediate_grabs = false) noexcept {
        Request out{};
        constexpr std::uint16_t mask = 0x2000 | 0x8000;
        if (!gameplay_fresh || (clocked_ && now < last_)) { cancel(); return out; }
        if (conflicting) {
            cancel(); clocked_ = true; last_ = now;
            previous_ = static_cast<std::uint16_t>(buttons & mask);
            // Track releases while another owner is active. A neutral B does
            // not need another release after a completed Ollie hands off.
            armed_ = previous_ == 0;
            return out;
        }
        clocked_ = true; last_ = now;
        const auto held = static_cast<std::uint16_t>(buttons & mask);
        const auto edges = static_cast<std::uint16_t>(held & ~previous_);
        previous_ = held;
        if (!armed_) { if (!held) armed_ = true; return out; }
        const auto direction = classic_direction(buttons, lx, ly);
        if (direction != previous_direction_) {
            if (direction != TrickDirection::none) {
                const bool repeated = (direction == TrickDirection::up || direction == TrickDirection::down) &&
                    previous_direction_ == TrickDirection::none && direction == last_direction_ &&
                    now - direction_at_ < (immediate_grabs ? 300u : 350u);
                direction_count_ = repeated ? 2 : 1;
                last_direction_ = direction; direction_at_ = now;
                direction_consumed_ = false;
            }
            previous_direction_ = direction;
        }
        if (held == mask) { count_ = 0; out.rejected = true; armed_ = false; return out; }
        if (immediate_grabs && (edges & 0x2000)) {
            const auto matched = direction == TrickDirection::none && !direction_consumed_ &&
                now - direction_at_ < 500 ? last_direction_ : direction;
            direction_consumed_ = true;
            immediate_count_ = immediate_count_ && matched == immediate_direction_ && now - immediate_at_ < 300
                ? std::min(immediate_count_ + 1, 3u) : 1;
            immediate_direction_ = matched; immediate_at_ = now; count_ = 0;
            const auto trick = select_named_trick(TrickFamily::grab, matched, immediate_count_);
            return {true, trick == ArcadeTrick::none, TrickFamily::grab, trick, matched, immediate_count_, 1};
        }
        // A late poll may close one command and start the next. Do not discard
        // the completed request just because the new edge arrived after its window.
        if (count_ && now - tap_at_ > 180) out = finish(rail);
        if (edges) {
            const auto family = edges & 0x2000 ? TrickFamily::grab : TrickFamily::grind;
            if (count_ && (family != family_ || direction != direction_ || now - tap_at_ > 180 || count_ >= 2)) {
                count_ = 0; out.rejected = true; armed_ = false; return out;
            }
            if (!count_) {
                family_ = family; direction_ = direction;
                direction_presses_ = direction == TrickDirection::none || now - direction_at_ > 350 ? 1 : direction_count_;
                if (family == TrickFamily::grind) direction_count_ = 1;
            }
            ++count_; tap_at_ = now;
        }
        if (count_ && now - tap_at_ >= 180) out = finish(rail);
        return out;
    }
private:
    Request finish(RailRelationship rail) noexcept {
        const auto trick = family_ == TrickFamily::grab ? select_named_trick(family_, direction_, count_) :
            select_grind(direction_, count_, direction_presses_, rail);
        const Request out{true, trick == ArcadeTrick::none, family_, trick, direction_, count_, direction_presses_};
        count_ = 0;
        return out;
    }
    bool armed_{true}, clocked_{};
    std::uint16_t previous_{};
    std::uint64_t last_{}, tap_at_{}, direction_at_{};
    unsigned count_{}, direction_count_{1}, direction_presses_{1};
    unsigned immediate_count_{};
    std::uint64_t immediate_at_{};
    TrickDirection immediate_direction_{};
    bool direction_consumed_{true};
    TrickFamily family_{};
    TrickDirection direction_{}, previous_direction_{}, last_direction_{};
};
}
