#pragma once
#include "arcade_classic_input.h"
#include "Extension/ArcadeMode/arcade_combo_session.h"

namespace dingosdk::arcade_mode::baseline {
inline SkatePose manual_input_pose(SkatePose candidate, bool native_pushing, bool established_balance,
        SkatePose physical = SkatePose::unknown) noexcept {
    // Presentation freshness must not erase valid physical input context.
    // The caller uses the existing owner-checked native wheel-contact reader;
    // combo/HUD identity admission remains independent and unchanged.
    if (candidate == SkatePose::unknown) candidate = physical;
    // A native push also lifts one wheel. Until independent balance is
    // established, its kick must not consume or cancel a fresh manual command.
    return native_pushing && !established_balance &&
        (candidate == SkatePose::manual || candidate == SkatePose::nose_manual) ? SkatePose::riding : candidate;
}
// Preparation is input recognition only; state101 never authorizes actuation.
inline bool manual_revert_preparation(bool valid, bool bail, unsigned state,
        unsigned mode, bool revert_pending) noexcept {
    return valid && !bail && state == 101 && mode == 1 && revert_pending;
}
// The measured native method holds a partial vertical right-stick deflection.
// This class requests that method; contact/score observations prove the pose.
class ManualGesture {
public:
    static constexpr std::uint64_t ground_pair_ms = 220, air_pair_ms = 400;
    // A natural250ms pair can span eight30Hz samples (267ms, or272ms with
    // rounded34ms polls). Keep the220ms reference plus a55ms usability/sampling
    // allowance. This never extends request lifetime or native actuation.
    static constexpr std::uint64_t ground_sample_slack_ms = 55;
    static constexpr std::uint64_t air_request_ms = 700;
    // confirmed remembers a native match for this request; it is not a fresh
    // pose event and must never replace the combo's independent native observer.
    struct Output { bool owns_stick{}, confirmed{}; std::int16_t y{}; bool suppress_reverse{}; };
    // Numeric snapshots travel through the existing bounded input telemetry.
    // They explain recognition/ownership only; native history proves success.
    struct Diagnostic {
        int stick{}, pad{}, stick_first{}, pad_first{}, request{};
        std::uint64_t stick_at{}, pad_at{}, request_at{};
        unsigned rejected{};
        bool safe{}, preparation{}, owns{}, confirmed{};
        bool operator==(const Diagnostic&) const = default;
    };
    const Diagnostic& diagnostic() const noexcept { return diagnostic_; }
    void cancel() noexcept {
        cancel_intent();
        clocked_ = false; stick_pair_ = {}; pad_pair_ = {};
    }
    Output poll(std::uint64_t now, std::uint16_t buttons, std::int16_t lx, std::int16_t ly,
                bool game_input, bool safe_state, SkatePose observed, bool conflicting,
                bool prepare_only = false) noexcept {
        const unsigned rejected = !game_input ? 1 : conflicting ? 2 :
            ((buttons & 0xf030u) || (buttons & 0x0300u) == 0x0300u) ? 3 :
            (!safe_state && !prepare_only) ? 4 :
            (observed == SkatePose::bail || observed == SkatePose::grind) ? 5 : 0;
        const int stick = stick_pair_.direction(lx, ly);
        const int pad = (buttons & 3u) == 1 ? 1 : (buttons & 3u) == 2 ? -1 : 0;
        const auto result = poll_impl(now, buttons, lx, ly, game_input, safe_state, observed, conflicting, prepare_only);
        diagnostic_ = {stick, pad, stick_pair_.first, pad_pair_.first, requested_,
            stick_pair_.at, pad_pair_.at, requested_ ? requested_at_ : 0,
            rejected, safe_state, prepare_only, result.owns_stick, result.confirmed};
        return result;
    }
private:
    void cancel_intent() noexcept {
        confirmed_ = air_prepared_ = landed_ = wrong_contact_ = false;
        requested_ = 0;
        stick_pair_.first = pad_pair_.first = 0;
        stick_pair_.at = pad_pair_.at = 0;
    }
    Output poll_impl(std::uint64_t now, std::uint16_t buttons, std::int16_t lx, std::int16_t ly,
                bool game_input, bool safe_state, SkatePose observed, bool conflicting,
                bool prepare_only) noexcept {
        const bool command_conflict = (buttons & 0xf030u) || (buttons & 0x0300u) == 0x0300u;
        const bool can_prepare = prepare_only && !requested_ && observed == SkatePose::riding;
        if (!game_input || (buttons & 0x0030u) ||
            (buttons & 3u) == 3u || (buttons & 12u) == 12u ||
            observed == SkatePose::bail || observed == SkatePose::grind ||
            (clocked_ && now < last_)) {
            cancel(); return {};
        }
        clocked_ = true; last_ = now;
        if ((!safe_state && !can_prepare) || conflicting || command_conflict) {
            // Rejected commands never retain preparation or synthetic ownership.
            // Within a known riding/air input session, still observe the physical
            // baseline: releasing a brake into a fresh direction is a new edge,
            // while a direction held through the gap is not. External context
            // resets and invalid gameplay above still discard all continuity.
            if (observed != SkatePose::riding && observed != SkatePose::air) {
                cancel(); return {};
            }
            cancel_intent();
            stick_pair_.baseline(stick_pair_.direction(lx, ly));
            const auto pad = buttons & 3u;
            pad_pair_.baseline(pad == 1 ? 1 : pad == 2 ? -1 : 0);
            return {};
        }
        // A native manual already in progress uses Up/Down for correction.
        // Unknown pose cannot establish that a fresh command is appropriate.
        if (!requested_ && observed != SkatePose::riding && observed != SkatePose::air) {
            cancel(); return {};
        }
        if (requested_) {
            // Recognition, airborne request lifetime and native landing settling
            // have separate deadlines. Rebounds cannot renew any of them.
            if (air_prepared_ && !landed_ && !confirmed_) {
                if (now - requested_at_ >= air_request_ms) { cancel(); return {}; }
                if (observed != SkatePose::air) { landed_ = true; requested_at_ = now; }
            }
            const auto wanted = requested_ < 0 ? SkatePose::manual : SkatePose::nose_manual;
            const auto opposite = requested_ < 0 ? SkatePose::nose_manual : SkatePose::manual;
            // A prepared landing can briefly touch one wheel then rebound. The
            // fixture measured17ms contact followed by150ms air. Retain only
            // that bounded initial landing request; intentional trick commands
            // still cancel above, and established manual takeoffs release.
            const bool settling_air = confirmed_ && air_prepared_ && observed == SkatePose::air &&
                now - first_confirmed_at_ < 250;
            // The Nose Manual fixture first touched the opposite wheel17ms,
            // rebounded200ms, then reached the requested pose. Keep only that
            // initial unconfirmed preparation; never call the wrong pose a match.
            if (observed == opposite && air_prepared_ && !confirmed_ && !wrong_contact_) {
                wrong_contact_ = true; wrong_contact_at_ = now;
            }
            const bool brief_wrong_contact = air_prepared_ && !confirmed_ && wrong_contact_ &&
                now - wrong_contact_at_ < 50;
            if ((observed == opposite && !brief_wrong_contact) ||
                (wrong_contact_ && !confirmed_ && now - wrong_contact_at_ >= 250) ||
                (confirmed_ && observed == SkatePose::air && !settling_air)) {
                cancel(); return {};
            }
            if (observed == wanted) {
                if (!confirmed_) first_confirmed_at_ = now;
                confirmed_ = true; confirmed_at_ = now;
            }
            // Brief observer gaps must not drop a held native manual. A failed
            // request or an observed exit always has a bounded release path.
            if ((!confirmed_ && now - requested_at_ >= 1200) ||
                (confirmed_ && now - confirmed_at_ >= (settling_air ? 200u : 150u))) {
                cancel(); return {};
            }
            return {true, confirmed_, static_cast<std::int16_t>(requested_ * 20000)};
        }
        const auto pad = buttons & 15u;
        const auto pair_ms = observed == SkatePose::air ? air_pair_ms : ground_pair_ms + ground_sample_slack_ms;
        // A held steering stick cannot swallow a fresh D-pad Up edge, and
        // switching input devices cannot complete half of an unrelated pair.
        const int pad_request = pad_pair_.poll(now, (pad & 1u) ? 1 : (pad & 2u) ? -1 : 0, pair_ms);
        if (pad) stick_pair_ = {};
        const int stick_request = pad ? 0 : stick_pair_.poll(now, stick_pair_.direction(lx, ly), pair_ms);
        const int request = pad_request ? pad_request : stick_request;
        if (request) {
            if (!safe_state) { cancel(); return {}; }
            requested_ = request; requested_at_ = now;
            air_prepared_ = observed == SkatePose::air; landed_ = !air_prepared_;
            return {true, false, static_cast<std::int16_t>(requested_ * 20000)};
        }
        // Native LS Down starts state101 before a rapid Down/Up can complete
        // (Deck1792220511391760000). Reserve only that grounded first tap;
        // an expired hold passes through and established balance is untouched.
        return {false, false, 0, observed == SkatePose::riding && ly < -11000 && stick_pair_.awaiting_up(now)};
    }
private:
    struct DirectionPair {
        bool armed{};
        int first{}, previous{};
        std::uint64_t at{};
        void baseline(int vertical) noexcept {
            armed = true; previous = vertical;
        }
        int direction(short x, short y) const noexcept {
            const int vertical = std::abs(static_cast<int>(y));
            const int horizontal = std::abs(static_cast<int>(x));
            const int sign = y > 0 ? 1 : y < 0 ? -1 : 0;
            // Accept ordinary diagonal thumb travel, with hysteresis around
            // the vertical threshold. Sideways steering alone is not a tap.
            const int threshold = sign == previous ? 8000 : 11000;
            return vertical >= threshold && vertical * 4 >= horizontal * 3 ? sign : 0;
        }
        bool awaiting_up(std::uint64_t now) const noexcept {
            return armed && first == -1 && previous == -1 && now >= at && now - at <= ground_pair_ms + ground_sample_slack_ms;
        }
        int poll(std::uint64_t now, int vertical, std::uint64_t window) noexcept {
            if (!armed) {
                // Baseline a held direction without treating it as an edge.
                // Subsequent fresh reversals must rearm even without neutral.
                armed = true;
                previous = vertical; return 0;
            }
            if (first && now - at > window) first = 0;
            int request{};
            if (vertical && vertical != previous) {
                if (first == -vertical && now - at <= window) request = vertical;
                first = vertical; at = now;
            }
            // A thumb can cross center on an arc. Sideways/neutral samples
            // neither erase nor refresh the bounded first vertical event.
            previous = vertical;
            return request;
        }
    };
    DirectionPair stick_pair_{}, pad_pair_{};
    Diagnostic diagnostic_{};
    bool clocked_{}, confirmed_{}, air_prepared_{}, landed_{}, wrong_contact_{};
    int requested_{};
    std::uint64_t last_{}, requested_at_{}, confirmed_at_{}, first_confirmed_at_{}, wrong_contact_at_{};
};
}
