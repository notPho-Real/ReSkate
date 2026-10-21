#pragma once
#include "arcade_combo_session.h"
#include "arcade_input_policy.h"

namespace dingosdk::arcade_mode {
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
struct ManualInputDiagnostic {
        int stick{}, pad{}, stick_first{}, pad_first{}, request{};
        std::uint64_t stick_at{}, pad_at{}, request_at{};
        unsigned rejected{};
        bool safe{}, preparation{}, owns{}, confirmed{};
        std::uint64_t recognized_at{};
        bool from_air{};
        bool operator==(const ManualInputDiagnostic&) const = default;
    };
// Native actuation only. Recognition and physical continuity belong to the
// controller session's shared journal. Contact/history still prove the result.
class ManualActuation {
public:
    struct Output { bool owns_stick{}, confirmed{}; std::int16_t y{}; bool suppress_reverse{}; };
    static constexpr auto ground_pair_ms = InputTiming::ground_pair_ms, ground_sample_slack_ms = InputTiming::ground_sample_slack_ms;
    static constexpr auto air_pair_ms = InputTiming::air_pair_ms, air_request_ms = InputTiming::air_request_ms;
    static constexpr std::uint64_t acquisition_ms = 1200;
    void cancel() noexcept { confirmed_ = air_prepared_ = landed_ = wrong_contact_ = clocked_ = false; requested_ = 0; hard_reset_ = true; }
    bool hard_reset() const noexcept { return hard_reset_; }
    bool busy() const noexcept { return requested_ != 0; }
    int requested() const noexcept { return requested_; }
    std::uint64_t recognized_at() const noexcept { return requested_ ? recognized_at_ : 0; }
    bool from_air() const noexcept { return requested_ && air_prepared_; }
    std::uint64_t requested_at() const noexcept { return requested_ ? requested_at_ : 0; }
    bool request(std::uint64_t now, int direction, bool airborne, bool safe,
                 bool prepared_air, std::uint64_t recognized_at) noexcept {
        if (!safe || requested_ || (direction != -1 && direction != 1)) return false;
        if (prepared_air && (now < recognized_at || now - recognized_at > air_request_ms)) return false;
        hard_reset_ = false; requested_ = direction;
        // Airborne waiting never renews preparation lifetime. A first admitted
        // ground execution begins the separate existing landing-settling clock.
        requested_at_ = prepared_air && airborne ? recognized_at : now;
        recognized_at_ = recognized_at;
        air_prepared_ = prepared_air; landed_ = !airborne;
        confirmed_ = wrong_contact_ = false;
        return true;
    }
    Output sample(std::uint64_t now, bool gameplay, bool safe, SkatePose observed,
                  bool conflicting, std::uint16_t buttons, bool balance_admitted = false,
                  bool trigger_only_conflict = false) noexcept {
        // Typed, independently admitted balance owns these configured vertical
        // held controls. Opposed command recognition remains a separate guard;
        // it must not drop the established native RS balance hold.
        const auto wanted=requested_<0 ? SkatePose::manual : SkatePose::nose_manual;
        const bool vertical_balance=balance_admitted && requested_ && confirmed_ && observed==wanted;
        const bool opposed_vertical=(buttons&3u)==3u && !vertical_balance;
        if (!gameplay || !safe || (conflicting && !(vertical_balance && trigger_only_conflict)) || (buttons & 0xf030u) || (buttons & 0x0300u) == 0x0300u ||
            opposed_vertical || (buttons & 12u) == 12u || observed == SkatePose::bail ||
            observed == SkatePose::grind || (clocked_ && now < last_)) {
            const bool hard = !gameplay || (buttons & 0x0030u) || opposed_vertical || (buttons & 12u) == 12u ||
                observed == SkatePose::bail || observed == SkatePose::grind || (clocked_ && now < last_);
            cancel(); hard_reset_ = hard; return {};
        }
        clocked_ = true; last_ = now;
        if (requested_) {
            // Recognition, airborne request lifetime and native landing settling
            // have separate deadlines. Rebounds cannot renew any of them.
            if (air_prepared_ && !landed_ && !confirmed_) {
                if (now - requested_at_ > air_request_ms) { cancel(); return {}; }
                // Missing observation is neither ground nor wheel contact. It
                // cannot turn the recognition deadline into a fresh lease.
                if (observed == SkatePose::riding || observed == SkatePose::manual ||
                    observed == SkatePose::nose_manual) { landed_ = true; requested_at_ = now; }
            }
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
            // Physical contact alone cannot waive the preparation bound. Only
            // independently admitted balance can maintain a continuing hold;
            // admitted corrected balances have no arbitrary lifespan limit.
            if (((!confirmed_ || !balance_admitted) && now - requested_at_ >= acquisition_ms) ||
                (confirmed_ && now - confirmed_at_ >= (settling_air ? 200u : 150u))) {
                cancel(); return {};
            }
            return {true, confirmed_, static_cast<std::int16_t>(requested_ * 20000)};
        }
        return {};
    }
private:
    bool hard_reset_{}, clocked_{}, confirmed_{}, air_prepared_{}, landed_{}, wrong_contact_{};
    int requested_{};
    std::uint64_t last_{}, requested_at_{}, recognized_at_{}, confirmed_at_{}, first_confirmed_at_{}, wrong_contact_at_{};
};
}
