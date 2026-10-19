#include "Extension/ArcadeMode/arcade_manual_labels.h"
#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
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
    ManualHistory history;
    ManualHistoryLabels labels;
    ManualObservation identity;
    ManualObservationContext context{1000, 1, true, true, true, false, false, SkatePose::riding};
    Fixture() { sample(1000, {}, SkatePose::riding); }
    ManualObservationResult sample(std::uint64_t now, std::span<const ManualHistoryRow> rows, SkatePose raw) {
        context.now = now;
        labels.observe(history.observe(rows, {now, 1, 2, 3, true, true}), now);
        return identity.poll(raw, labels.take(now), context);
    }
};
}
int main() {
    for (bool nose : {false, true}) {
        Fixture f;
        const auto wanted = nose ? SkatePose::nose_manual : SkatePose::manual;
        const std::array rows{ManualHistoryRow{10, 20, nose ? 2u : 1u}};
        const auto native = NativeComboObservation{true, true, false, false, 100, 1, nose ? 0x8000u : 0x4000u};
        f.context.contact = manual_contact_pose(native, false);
        check(observed_pose(native) == SkatePose::riding,
              "generic scoring inactivity suppresses the old candidate despite valid physical contact");
        check(!f.sample(1017, {}, observed_pose(native)).confirmed,
              "physical contact with inactive generic scoring is still insufficient alone");
        const auto result = f.sample(1050, rows, observed_pose(native));
        check(result.admitted && result.pose == wanted,
              "independent fresh matching history and physical contact establish manual identity without generic scoring activity");
    }
    for (bool nose : {false, true}) for (auto delay : {0u, 1u, 17u, 50u, 150u}) {
        Fixture f;
        const auto wanted = nose ? SkatePose::nose_manual : SkatePose::manual;
        const std::array rows{ManualHistoryRow{10, 20, nose ? 2u : 1u}};
        f.context.contact = SkatePose::unknown;
        check(!f.sample(1050, rows, SkatePose::air).confirmed,
              "a history edge alone cannot establish physical manual contact");
        f.context.contact = wanted;
        for (unsigned elapsed = 17; elapsed < delay; elapsed += 17) {
            f.context.contact = SkatePose::unknown;
            check(!f.sample(1050 + elapsed, rows, SkatePose::air).confirmed,
                  "pending identity alone grants no balance during approach");
        }
        f.context.contact = wanted;
        const auto result = f.sample(1050 + delay, rows, wanted);
        check(result.admitted && result.confirmed && result.pose == wanted,
              "fresh independently observed history survives until matching next-tick landing contact");
        ComboSession combo;
        combo.step(SkatePose::air, 0, 0);
        combo.step(result.pose, .017f, 0);
        check(balance_meter(combo.balance_display_state()).visible,
              "ordered native identity/contact activates the balance lifecycle");
        check(!f.sample(1067 + delay, rows, wanted).admitted,
              "consumed correlation cannot award another manual section");
        f.identity.reset();
        check(!f.sample(1084 + delay, rows, wanted).confirmed,
              "reset preserves consumed identity and clears pending correlation");
    }
    for (unsigned fault = 0; fault < 13; ++fault) {
        Fixture f;
        const std::array rows{ManualHistoryRow{10, 20, 1}};
        f.context.contact = SkatePose::unknown;
        f.sample(1050, rows, SkatePose::air);
        if (fault == 0) f.context.owner_changed = true;
        if (fault == 1) ++f.context.generation;
        if (fault == 2) f.context.valid = false;
        if (fault == 3) f.context.enabled = false;
        if (fault == 4) f.context.gap = true;
        if (fault == 5) f.context.activity_fresh = false;
        if (fault == 6) f.identity.reset();
        if (fault == 7) {
            f.context.contact = SkatePose::nose_manual;
            f.sample(1060, rows, SkatePose::nose_manual);
        }
        if (fault == 8) f.sample(1060, rows, SkatePose::grind);
        if (fault == 9) f.sample(1060, rows, SkatePose::bail);
        if (fault == 10) {
            for (unsigned elapsed = 17; elapsed <= 153; elapsed += 17)
                f.sample(1050 + elapsed, rows, SkatePose::air);
        }
        if (fault == 12) {
            f.context.contact = SkatePose::riding;
            f.sample(1055, rows, SkatePose::riding);
            f.context.contact = SkatePose::unknown;
            f.sample(1060, rows, SkatePose::air);
        }
        f.context.contact = fault == 11 ? SkatePose::unknown : SkatePose::manual;
        check(!f.sample(fault == 10 ? 1220 : 1067, rows, SkatePose::manual).confirmed,
              "pending history never crosses context, stale age, opposite contact or missing physical corroboration");
        f.context = {1240, 1, true, true, true, false, false, SkatePose::riding};
        f.sample(1240, rows, SkatePose::riding);
        f.context.contact = SkatePose::manual;
        check(!f.sample(1257, rows, SkatePose::manual).confirmed,
              "discarded correlation cannot revive from persistent history");
    }
    std::cout << checks << " manual admission ordering checks passed\n";
}
