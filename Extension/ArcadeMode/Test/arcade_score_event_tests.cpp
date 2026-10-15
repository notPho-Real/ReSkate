#include "Extension/ArcadeMode/arcade_score_event.h"
#include <cstdlib>
#include <iostream>

namespace {
unsigned checks{};
void check(bool passed, const char* description) {
    ++checks;
    if (!passed) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
}

int run_arcade_score_event_tests() {
    using namespace dingosdk::arcade_mode;
    constexpr auto fs = score_label_bit(3), bs = score_label_bit(4);
    ScoreLabelEdges edges;
    check(!edges.take(0).valid, "unobserved labels cannot confirm a native outcome");
    edges.observe(fs, true, 10);
    auto result = edges.take(10);
    check(result.valid && !result.labels && result.uncertainty == ScoreLabelUncertainty::baseline,
        "a preexisting revert label at startup is only a baseline");
    edges.observe(fs, true, 60);
    check(!edges.take(60).labels, "persistent label rereads do not produce occurrences");
    edges.observe(0, true, 110);
    check(!edges.take(110).labels, "an observed clear does not itself become a trick");
    edges.observe(fs, true, 160);
    result = edges.take(160);
    check(result.valid && result.contains(3) && !result.contains(4) &&
        result.sequence == 1 && result.observed_at == 160 && result.sampled_at == 160 &&
        result.uncertainty == ScoreLabelUncertainty::none,
        "a clear followed by FS Revert emits one independent sanitized label edge");
    check(!edges.take(160).labels, "the same occurrence is drained exactly once");
    edges.observe(fs, true, 210);
    check(!edges.take(210).labels, "a label present in another slot has the same aggregate identity");
    edges.observe(bs, true, 260);
    result = edges.take(260);
    check(result.contains(4) && !result.contains(3) && result.sequence == 2,
        "an actual FS to BS label change emits only the new identity");
    edges.observe(0, false, 310);
    check(!edges.take(310).valid, "an incomplete read cannot masquerade as a clear");
    edges.observe(bs, true, 360);
    check(!edges.take(360).labels, "recovery after an unavailable sample does not replay an old label");
    edges.observe(0, true, 410);
    edges.observe(fs, true, 460);
    edges.observe(fs, true, 610);
    result = edges.take(611);
    check(!result.valid && !result.labels && result.uncertainty == ScoreLabelUncertainty::stale,
        "fresh rereads cannot make an undrained stale occurrence fresh again");
    edges.observe(0, true, 700);
    edges.observe(fs, true, 1000);
    check(!edges.take(1000).labels, "a sampling gap cannot establish when a label first appeared");
    edges.observe(0, true, 1050);
    edges.observe(fs | bs, true, 1100);
    result = edges.take(1100);
    check(result.contains(3) && result.contains(4) && result.uncertainty == ScoreLabelUncertainty::coalesced,
        "simultaneous different label edges explicitly retain ambiguity");
    edges.observe(0, true, 1150);
    edges.observe(fs, true, 1200);
    edges.observe(0, true, 1250);
    edges.observe(fs, true, 1300);
    check(edges.take(1300).uncertainty == ScoreLabelUncertainty::coalesced,
        "multiple undrained occurrences cannot silently collapse to one proven trick");
    edges.observe(bs, true, 1299);
    check(!edges.take(1299).labels, "backwards observation clocks rebaseline without phantom labels");
    edges.observe(0, true, 1350);
    edges.observe(fs, true, 1400);
    result = edges.take(1399);
    check(!result.valid && !result.labels && result.uncertainty == ScoreLabelUncertainty::clock_reset,
        "backwards consumer clocks invalidate pending occurrences");
    edges.observe(1u << 31, true, 1500);
    check(!edges.take(1500).valid, "out-of-allowlist numeric labels are invalid evidence");
    check(!score_label_bit(0) && !score_label_bit(21) && !result.contains(32),
        "invalid diagnostic IDs cannot overflow bit selection");
    check(score_label_id("FS Revert") == 3 && score_label_id("BS Revert") == 4 &&
        score_label_id("FS Revert private text") == 0,
        "exact existing label allowlist remains the only text-to-ID conversion");
    check(score_label_id("Switch FS Revert") == 3 && score_label_id("Switch FS Revert private text") == 0,
        "observed switch revert spelling retains exact matching and semantic identity");
    check(score_label_id("Switch BS Revert") == 4 && score_label_id("Switch BS Revert private text") == 0,
        "observed switch backside spelling retains exact matching and semantic identity");
    std::cout << checks << " score label occurrence checks passed\n";
    return 0;
}

#ifndef ARCADE_SCORE_EVENT_COMBINED_TESTS
int main() { return run_arcade_score_event_tests(); }
#endif
