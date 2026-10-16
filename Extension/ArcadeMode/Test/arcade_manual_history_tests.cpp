#include "Extension/ArcadeMode/arcade_manual_history.h"
#include "Extension/ArcadeMode/arcade_manual_labels.h"
#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include <cstdlib>
#include <iostream>
#include <type_traits>

using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* message) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
ManualHistoryContext context(std::uint64_t now = 1000) { return {now, 100, 200, 300, true, true}; }
bool no_delta(const ManualHistoryDiagnostic& d) {
    return !d.manual_added && !d.manual_removed && !d.nose_added && !d.nose_removed && !d.relabels;
}
}
int main() {
    static_assert(std::is_trivially_copyable_v<ManualHistoryDiagnostic>);
    static_assert(sizeof(ManualHistoryDiagnostic) <= 64);
    const ManualHistoryRow manual{1, 10, 1}, nose{2, 10, 2}, other{3, 10, 26};
    const std::array first{manual, nose, other};
    {
        ManualHistory history;
        const auto baseline = history.observe(first, context());
        check(baseline.available && !baseline.comparable && baseline.status == ManualHistoryStatus::baseline &&
              baseline.rows == 3 && baseline.distinct == 3 && baseline.manual_rows == 1 && baseline.nose_rows == 1 &&
              no_delta(baseline), "complete first snapshot counts labels without inventing additions");
        const std::array moved{other, manual, nose};
        const auto reorder = history.observe(moved, context(1050));
        check(reorder.comparable && reorder.status == ManualHistoryStatus::complete && no_delta(reorder),
              "moving presenters between slots creates no additions or removals");
        const std::array repeated{other, manual, nose, ManualHistoryRow{4, 10, 1}};
        const auto added = history.observe(repeated, context(1100));
        check(added.manual_rows == 2 && added.manual_distinct == 2 && added.manual_added == 1 &&
              !added.manual_removed && !added.nose_added && !added.relabels,
              "second same-label presenter is visible despite unchanged label union");
        const auto persisted = history.observe(repeated, context(1150));
        check(persisted.comparable && no_delta(persisted), "persistent history is not another addition");
        const auto removed = history.observe(first, context(1200));
        check(removed.manual_removed == 1 && !removed.manual_added, "removal is independently visible");
    }
    {
        ManualHistory history; history.observe(first, context());
        const std::array duplicates{manual, other, manual, nose};
        const auto d = history.observe(duplicates, context(1050));
        check(d.comparable && d.rows == 4 && d.distinct == 3 && d.manual_rows == 2 &&
              d.manual_distinct == 1 && d.duplicates == 1 && no_delta(d),
              "duplicate presenter slots cannot masquerade as distinct manual additions");
        const std::array conflict{manual, ManualHistoryRow{1, 10, 2}};
        const auto bad = history.observe(conflict, context(1100));
        check(!bad.available && !bad.comparable && bad.status == ManualHistoryStatus::invalid && bad.rows == 0,
              "conflicting labels for one presenter invalidate entire snapshot");
        check(history.observe(first, context(1150)).status == ManualHistoryStatus::baseline,
              "partial malformed view cannot become a comparison baseline");
    }
    {
        ManualHistory history; history.observe(first, context());
        const std::array changed{ManualHistoryRow{1, 10, 2}, nose, other};
        const auto d = history.observe(changed, context(1050));
        check(d.relabels == 1 && d.manual_removed == 1 && d.nose_added == 1 && d.manual_added == 0,
              "same presenter relabel is explicit rather than a native occurrence claim");
        const std::array recycled{ManualHistoryRow{1, 11, 2}, nose, other};
        const auto type = history.observe(recycled, context(1100));
        check(type.relabels == 0 && type.nose_removed == 1 && type.nose_added == 1,
              "changed presenter type is a replacement, never silently retained identity");
    }
    for (unsigned fault = 0; fault < 6; ++fault) {
        ManualHistory history; history.observe(first, context());
        auto c = context(1050);
        auto expected = ManualHistoryStatus::model_changed;
        if (fault == 0) c.manager++;
        if (fault == 1) c.model_handle++;
        if (fault == 2) c.model_type++;
        if (fault == 3) { c.now = 999; expected = ManualHistoryStatus::clock_reset; }
        if (fault == 4) { c.now = 1151; expected = ManualHistoryStatus::gap; }
        if (fault == 5) { history.reset(); expected = ManualHistoryStatus::baseline; }
        const auto d = history.observe(first, c);
        check(d.available && !d.comparable && d.status == expected && no_delta(d),
              "context change counts current view but forbids cross-context deltas");
        c.now += 50;
        check(history.observe(first, c).comparable, "new complete baseline supports later bounded comparison");
    }
    for (unsigned fault = 0; fault < 8; ++fault) {
        ManualHistory history; history.observe(first, context());
        auto c = context(1050); auto rows = first;
        auto expected = ManualHistoryStatus::invalid;
        if (fault == 0) c.complete = false;
        if (fault == 1) { c.history_source = false; expected = ManualHistoryStatus::fallback; }
        if (fault == 2) c.manager = 0;
        if (fault == 3) c.model_handle = 0;
        if (fault == 4) c.model_type = 0;
        if (fault == 5) rows[0].handle = 0;
        if (fault == 6) rows[0].type = 0;
        if (fault == 7) rows[0].label = 27;
        const auto d = history.observe(rows, c);
        check(!d.available && !d.comparable && d.status == expected && !d.rows && no_delta(d),
              "unavailable or malformed history remains explicitly unavailable, not zero evidence");
        check(history.observe(first, context(1100)).status == ManualHistoryStatus::baseline,
              "invalid or fallback history cannot bridge identity comparison");
    }
    {
        ManualHistory history;
        std::array<ManualHistoryRow, 17> rows{};
        for (unsigned i = 0; i < rows.size(); ++i) rows[i] = {i + 1u, 10, i % 3};
        const auto full = history.observe(std::span(rows).first(16), context());
        check(full.available && full.rows == 16 && full.distinct == 16, "exact fixed16-row limit supported");
        const auto overflow = history.observe(rows, context(1050));
        check(!overflow.available && overflow.status == ManualHistoryStatus::invalid,
              "17rows reject rather than truncate into misleading deltas");
        const auto empty = history.observe({}, context(1100));
        check(empty.available && empty.rows == 0 && empty.status == ManualHistoryStatus::baseline,
              "complete empty history differs from unavailable data");
        const auto filled = history.observe(first, context(1250));
        check(filled.comparable && filled.manual_added == 1 && filled.nose_added == 1,
              "exact150ms freshness boundary is comparable after a measured empty view");
        const auto cleared = history.observe({}, context(1300));
        check(cleared.comparable && cleared.manual_removed == 1 && cleared.nose_removed == 1,
              "actual complete history clear records removals without fabricating a bank");
    }
    // Recorded HIL51 second-manual additions are 350/383ms after landing.
    // Replay each independent timing, without merging separate hardware trials.
    for (auto delay : {384u, 350u, 383u}) {
        ManualHistory history; ManualHistoryLabels labels; ManualObservation gate;
        const std::array initial{other};
        const std::array once{other, manual};
        const std::array twice{other, manual, ManualHistoryRow{4, 10, 1}};
        auto step = [&](std::uint64_t now, std::span<const ManualHistoryRow> rows, SkatePose raw) {
            labels.observe(history.observe(rows, context(now)), now);
            return gate.poll(raw, labels.take(now), {now, 1, true, true, true, false, false});
        };
        check(!step(1000, initial, SkatePose::riding).admitted, "initial history is only a baseline");
        check(step(1016, once, SkatePose::manual).admitted, "first matching history addition confirms native contact");
        check(step(1032, once, SkatePose::manual).confirmed, "persistent native contact remains confirmed");
        step(1048, once, SkatePose::air);
        step(1064, once, SkatePose::riding);
        for (unsigned elapsed = 16; elapsed < delay; elapsed += 16)
            check(!step(1064 + elapsed, once, SkatePose::manual).confirmed,
                  "second native contact waits for fresh corroboration despite old label presence");
        const auto second = step(1064 + delay, twice, SkatePose::manual);
        check(second.admitted && second.confirmed, "distinct same-label addition confirms the second manual");
        check(!step(1080 + delay, twice, SkatePose::manual).admitted, "persistent second presenter cannot duplicate admission");
        step(1096 + delay, twice, SkatePose::air);
        check(!step(1112 + delay, twice, SkatePose::manual).confirmed, "old two labels cannot confirm a third manual");
    }
    {
        ManualHistory history; ManualHistoryLabels labels;
        const std::array baseline{other};
        const std::array once{other, manual};
        const std::array twice{other, manual, ManualHistoryRow{4,10,1}};
        labels.observe(history.observe(baseline, context()), 1000); labels.take(1000);
        labels.observe(history.observe(once, context(1016)), 1016);
        labels.observe(history.observe(twice, context(1032)), 1032);
        const auto coalesced = labels.take(1032);
        check(coalesced.valid && coalesced.sequence == 2 && coalesced.uncertainty == ScoreLabelUncertainty::coalesced,
              "two unread same-label additions remain explicitly ambiguous");
        const std::array moved{twice[2], other, manual};
        labels.observe(history.observe(moved, context(1048)), 1048);
        check(labels.take(1048).labels == 0, "slot movement creates no manual evidence");
        labels.observe(history.observe(once, context(1064)), 1064);
        check(!labels.take(1064).valid, "presenter removal invalidates pending evidence");
        labels.observe(history.observe(twice, context(1080)), 1080);
        check(!labels.take(1080).labels, "reappearance after removal supplies a baseline, not a new manual");
    }
    for (unsigned fault = 0; fault < 9; ++fault) {
        ManualHistory history; ManualHistoryLabels labels;
        const std::array baseline{other};
        labels.observe(history.observe(baseline, context()), 1000); labels.take(1000);
        const std::array added{other, manual};
        auto diagnostic = history.observe(added, context(1016));
        if (fault == 0) diagnostic.duplicates = 1;
        if (fault == 1) diagnostic.relabels = 1;
        if (fault == 2) diagnostic.manual_removed = 1;
        if (fault == 3) diagnostic.available = false;
        if (fault == 4) diagnostic.comparable = false;
        if (fault == 5) diagnostic.status = ManualHistoryStatus::model_changed;
        if (fault == 6) diagnostic.manual_added = 17;
        const auto now = fault == 7 ? 999u : fault == 8 ? 1151u : 1016u;
        labels.observe(diagnostic, now);
        const auto rejected = labels.take(now);
        check(!rejected.valid || !rejected.labels || rejected.uncertainty != ScoreLabelUncertainty::none,
              "ambiguous replacement, context or time cannot corroborate a manual");
    }
    {
        ManualHistory history; ManualHistoryLabels labels;
        labels.observe(history.observe({}, context()), 1000); labels.take(1000);
        const std::array added{nose};
        labels.observe(history.observe(added, context(1016)), 1016);
        const auto exact = labels.take(1016);
        check(exact.valid && exact.labels == score_label_bit(2), "nose addition retains exact label");
        labels.observe(history.observe(added, context(1032)), 1032);
        check(!labels.take(1183).valid, "stale history cannot retain valid evidence");
        labels.invalidate();
        labels.observe(history.observe(added, context(1199)), 1199);
        check(!labels.take(1199).labels, "toggle/reset cannot replay a persistent history label");
    }
    std::cout << checks << " manual history diagnostic checks passed\n";
}
