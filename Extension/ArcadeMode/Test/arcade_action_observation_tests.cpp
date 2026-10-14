#include "Extension/ArcadeMode/arcade_action_observation.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
unsigned checks{};
void check(bool value, const char* text) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << text << '\n'; std::exit(1); }
}
}

int run_arcade_action_observation_tests() {
    using namespace dingosdk::arcade_mode;
    using Kind = ObservedActionKind;
    ActionObservationHandoff handoff;
    check(!handoff.request(1, Kind::flip, 2, 0, 0), "uninitialized observer rejects requests");
    check(handoff.reset(1, 100, 10), "new session establishes native watermark");
    check(!handoff.request(1, Kind::none, std::nullopt, 100, 10), "unknown requested family is rejected");
    check(!handoff.request(1, Kind::flip, 0, 100, 10) && !handoff.request(1, Kind::flip, 64, 100, 10),
        "unestablished exact flip identifiers are rejected");
    check(!handoff.request(1, Kind::grab, std::nullopt, 100, 10, 3001) &&
        !handoff.request(1, Kind::grab, std::nullopt, 100, 10, 0), "request duration is bounded and nonzero");
    check(handoff.request(1, Kind::flip, 2, 110, 10), "exact flip request is retained without emitting success");
    check(!handoff.request(2, Kind::grab, std::nullopt, 110, 10), "pending request capacity is exactly one");
    NativeActionEvent flip{true, 1, 11, 120, Kind::flip, 3, SkatePose::air};
    auto out = handoff.observe(flip, 120);
    check(out.observed && out.native_id == 3 && !out.matched_request && handoff.pending(),
        "different native flip remains an independent observed mismatch");
    flip.sequence = 12; flip.ms = 130; flip.native_id = 2;
    out = handoff.observe(flip, 130);
    check(out.observed && out.matched_request && out.request_id == 1 && !handoff.pending(),
        "fresh exact native occurrence consumes the matching request");
    check(!handoff.observe(flip, 130).observed, "repeated native event is never emitted twice");
    check(!handoff.request(1, Kind::flip, 2, 140, 12), "reused request identifier is rejected");
    check(handoff.request(2, Kind::flip, 2, 140, 14), "request uses observer watermark even if events are queued");
    flip.sequence = 13; flip.ms = 140;
    out = handoff.observe(flip, 140);
    check(out.observed && !out.matched_request, "queued pre-request event cannot match an intent");
    flip.sequence = 15; flip.ms = 150;
    check(handoff.observe(flip, 150).matched_request, "next native occurrence beyond watermark matches");

    check(handoff.request(3, Kind::manual, std::nullopt, 160, 15, 100),
        "mechanic correlation can leave unestablished native identity unknown");
    NativeActionEvent manual{true, 1, 16, 170, Kind::manual, 0, SkatePose::manual};
    out = handoff.observe(manual, 170);
    check(out.observed && out.matched_request && out.native_id == 0 && !out.observed_revert,
        "manual observation does not fabricate a named identity or revert");
    check(handoff.request(4, Kind::revert, std::nullopt, 180, 16, 100), "revert intent begins a bounded request");
    check(handoff.advance(281) && !handoff.pending(), "request expires without inventing native success");
    NativeActionEvent revert{true, 1, 17, 282, Kind::revert, 0, SkatePose::riding};
    out = handoff.observe(revert, 282);
    check(out.observed_revert && !out.matched_request,
        "fresh independent revert is usable even without a pending request");

    ComboSession combo;
    combo.step(SkatePose::riding, 0.01f, 0);
    combo.step(SkatePose::air, 0.01f, 0);
    combo.step(SkatePose::riding, 0.01f, 0);
    combo.step(out.pose, 0.01f, 0, out.observed_revert);
    check(combo.model().state().revert_pending && combo.model().state().raw_points == 0,
        "observed revert plugs into existing grace model without invented points");
    out = handoff.observe(revert, 283);
    combo.step(SkatePose::riding, 0.01f, 0, out.observed_revert);
    check(!out.observed && combo.model().state().multiplier == 1,
        "duplicate revert cannot produce a second observation or multiplier");

    check(handoff.request(5, Kind::grab, std::nullopt, 290, 17), "pending request exists before native bail");
    NativeActionEvent bail{true, 1, 18, 300, Kind::flip, 0, SkatePose::bail};
    out = handoff.observe(bail, 300);
    check(out.bailed && !out.matched_request && !handoff.pending() && !handoff.active(),
        "native bail clears pending requests and closes the observation epoch");
    combo.step(out.pose, 0.01f, 0, out.observed_revert);
    check(!combo.model().state().combo_active && combo.model().state().fatigue == 0,
        "independent bail handoff clears the existing combo and fatigue");
    revert.sequence = 19; revert.ms = 301;
    check(!handoff.observe(revert, 301).observed, "late events cannot reopen a bailed epoch");
    check(!handoff.reset(1, 310, 19) && handoff.reset(2, 310, 19), "recovery requires a new epoch");
    revert.ms = 320; revert.sequence = 20;
    check(!handoff.observe(revert, 320).observed, "old epoch event is rejected after reset");
    revert.epoch = 2;
    check(handoff.observe(revert, 320).observed_revert, "new epoch can observe a fresh independent event");

    for (int invalid = 0; invalid < 5; ++invalid) {
        NativeActionEvent stale{true, 2, 21, 330, Kind::flip, 2, SkatePose::air};
        if (invalid == 0) stale.valid = false;
        if (invalid == 1) stale.ms = 501;
        if (invalid == 2) stale.ms = 349;
        if (invalid == 3) stale.sequence = 20;
        if (invalid == 4) stale.native_id = 64;
        check(!handoff.observe(stale, 500).observed, "invalid stale future or duplicate native evidence is rejected");
    }
    check(!handoff.advance(499) && !handoff.active(), "backwards clock closes the epoch");
    check(handoff.reset(3, std::numeric_limits<std::uint64_t>::max() - 20, 0), "session accepts a near-limit monotonic time");
    check(handoff.request(1, Kind::flip, 2, std::numeric_limits<std::uint64_t>::max() - 10, 0, 20),
        "duration arithmetic avoids overflowing an absolute deadline");
    flip = {true, 3, 1, std::numeric_limits<std::uint64_t>::max(), Kind::flip, 2, SkatePose::air};
    check(handoff.observe(flip, flip.ms).matched_request, "near-limit fresh event still matches");
    handoff.invalidate();
    check(!handoff.pending() && !handoff.observe(flip, flip.ms).observed, "explicit disable clears event handoff");
    ActionObservationHandoff queued;
    queued.reset(1, 100, 0);
    queued.request(1, Kind::grab, std::nullopt, 120, 0);
    NativeActionEvent earlier{true, 1, 1, 110, Kind::grab, 0, SkatePose::air};
    out = queued.observe(earlier, 120);
    check(out.observed && !out.matched_request && queued.pending(),
        "a delayed native event predating the request is not attributed to it");
    earlier.sequence = 2; earlier.ms = 121; earlier.kind = Kind::manual; earlier.pose = SkatePose::manual;
    out = queued.observe(earlier, 121);
    check(out.observed && !out.matched_request && queued.pending(),
        "different observed mechanic cannot satisfy a pending request");
    queued.reset(2, 130, 2);
    check(!queued.pending(), "explicit scene reset discards unconfirmed requests");
    std::cout << checks << " action observation checks passed\n";
    return 0;
}
#ifndef ARCADE_ACTION_OBSERVATION_COMBINED_TESTS
int main() { return run_arcade_action_observation_tests(); }
#endif
