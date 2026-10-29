#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_manual_labels.h"
#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* why) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
}
struct Fixture {
    InputSession session;
    ManualHistory history;
    ManualHistoryLabels labels;
    ManualObservation identity;
    ComboSession combo{native_combo_tuning()};
    NativeGesture::Output output{};
    ManualObservationResult observation{};
    std::uint64_t last{};
    bool failed{};
    NativeGesture::Output sample(std::uint64_t now, SkatePose raw, SkatePose contact,
            std::span<const ManualHistoryRow> rows = {}, std::int16_t y = 0,
            bool feedback = false, std::uint16_t buttons = 0, bool gameplay = true) {
        labels.observe(history.observe(rows, {now, 1, 2, 3, true, true}), now);
        observation = identity.poll(raw, labels.take(now),
            {now, 1, gameplay, true, true, false, false, contact});
        const auto& state = combo.model().state();
        const float correction = feedback ? std::clamp(-4 * state.balance - 2 * state.balance_velocity, -1.0f, 1.0f) : 0;
        const auto prior = state.balance_kind;
        failed = combo.step(observation.pose, last ? static_cast<float>(now - last) / 1000 : 0, correction);
        last = now;
        if (!gameplay || failed || (prior == BalanceKind::manual && balance_kind(observation.pose) != BalanceKind::manual))
            session.cancel_manual();
        InputAdmission admission{};
        admission.gameplay = gameplay; admission.authored = true; admission.manual_safe = true;
        admission.pose = balance_kind(observation.pose) == BalanceKind::manual ? observation.pose : raw;
        admission.tick = now / 17;
#ifndef ARCADE_MANUAL_LIFECYCLE_BASELINE
        admission.manual_balance_admitted = observation.confirmed && !combo.failed_section() &&
            combo.model().state().balance_kind == BalanceKind::manual;
#endif
        output = session.poll(now, {buttons, 0, y}, 0, 0, admission);
        return output;
    }
    void prepare(bool nose) {
        sample(1000, SkatePose::riding, SkatePose::riding);
        sample(1017, SkatePose::air, SkatePose::unknown, {}, static_cast<std::int16_t>(nose ? -25000 : 25000));
        check(sample(1034, SkatePose::air, SkatePose::unknown, {}, static_cast<std::int16_t>(nose ? 25000 : -25000)).manual.owns,
              "production session recognizes the airborne pair through its shared journal");
    }
};
}
int main() {
    for (bool nose : {false, true}) {
        Fixture f; f.prepare(nose);
        const auto wanted = nose ? SkatePose::nose_manual : SkatePose::manual;
        for (std::uint64_t now = 1051; now < 4000; now += 17) {
            f.sample(now, wanted, wanted);
            check(!balance_meter(f.combo.balance_display_state()).visible,
                  "unlabelled physical contact cannot fabricate the balance lifecycle");
        }
        check(!f.output.manual.owns && !f.output.right_y,
              "unadmitted contact cannot hold unified native actuation indefinitely");
    }
    for (bool nose : {false, true}) for (bool early : {false, true}) {
        Fixture f; f.prepare(nose);
        const auto wanted = nose ? SkatePose::nose_manual : SkatePose::manual;
        const std::array rows{ManualHistoryRow{10, 20, nose ? 2u : 1u}};
        if (early) {
            f.sample(1051, SkatePose::air, SkatePose::unknown, rows);
            check(!f.observation.confirmed && !balance_meter(f.combo.balance_display_state()).visible,
                  "an early history edge alone never activates balance");
        }
        for (std::uint64_t now = early ? 1068 : 1051; now < 61000; now += 17) {
            // Inactive generic scoring reports riding; independently validated
            // contact and a fresh history occurrence must still establish identity.
            f.sample(now, SkatePose::riding, wanted, rows, 0, true);
            check(f.observation.confirmed && !f.failed && f.output.manual.owns &&
                f.output.right_y == (nose ? 20000 : -20000) && balance_meter(f.combo.balance_display_state()).visible,
                  "legitimate history/contact ordering drives production actuation and visible corrected balance without a lifespan cap");
        }
        check(f.combo.model().state().manual_fatigue > 0,
              "admitted session owns a real correction/fatigue lifecycle");
        check(f.sample(61000, wanted, wanted, rows, 0, true, 0x2000).manual.owns,
              "ground B is grab-only and preserves admitted manual actuation");
        check(!f.sample(61017, wanted, wanted, rows, 0, true, 0, false).manual.owns,
              "gameplay loss releases the shared session");
    }
#ifndef ARCADE_MANUAL_LIFECYCLE_BASELINE
    for (bool admitted : {false, true}) {
        ArcadeInput input; input.enable(true);
        const auto sample = [&](std::uint64_t now, std::int16_t y, SkatePose pose) {
            return input.poll(0, now, 0, 0, y, 0, 0, true, false, false, 0, false,
                false, true, now / 17, false, {}, false, true, pose, false, false,
                false, false, true, false, 0, 0, ActionExperiment::off, false, false, admitted);
        };
        sample(1000, 0, SkatePose::air); sample(1017, 25000, SkatePose::air);
        check(sample(1034, -25000, SkatePose::air).manual.owns,
              "the remap facade routes the pair into the production session");
        NativeGesture::Output output{};
        for (std::uint64_t now = 1051; now < 5000; now += 17) output = sample(now, 0, SkatePose::manual);
        check(output.manual.owns == admitted && (output.right_y == -20000) == admitted,
              "the remap facade forwards independent balance authority without inferring it from pose");
        input.enable(false);
        check(!sample(5017, 0, SkatePose::manual).manual.owns, "OFF clears the production facade ownership");
    }
#endif
    std::cout << checks << " unified manual lifecycle checks passed\n";
}
