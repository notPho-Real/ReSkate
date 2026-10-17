#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include <cstdlib>
#include <iostream>

using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* message) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
ScoreLabelOccurrence empty(std::uint64_t sequence = 0) {
    return {true, 0, sequence, 0, 0, ScoreLabelUncertainty::none};
}
ScoreLabelOccurrence edge(std::uint64_t now, std::uint64_t sequence = 1, unsigned id = 1) {
    return {true, score_label_bit(id), sequence, now, now, ScoreLabelUncertainty::none};
}
struct Fixture {
    ManualObservation gate;
    ManualObservationContext context{1000, 7, true, true, true, false, false};
    Fixture() { gate.poll(SkatePose::riding, empty(), context); }
    ManualObservationResult poll(std::uint64_t now, SkatePose raw, ScoreLabelOccurrence label = empty()) {
        context.now = now; return gate.poll(raw, label, context);
    }
};
}
int main() {
    for (const bool nose : {false, true}) {
        Fixture f;
        const auto pose = nose ? SkatePose::nose_manual : SkatePose::manual;
        f.context.contact = pose;
        check(f.poll(1010, pose, edge(1010, 1, nose ? 2 : 1)).admitted,
              "fresh native identity admits a physical manual section");
        f.context.activity_fresh = false;
        for (unsigned ms = 1020; ms <= 7020; ms += 50) {
            const auto result = f.poll(ms, SkatePose::riding);
            check(result.confirmed && result.pose == pose && !result.admitted,
                  "fresh same-owner native contact retains its meter through score activity gaps without a new link");
        }
        f.context.contact = SkatePose::riding;
        check(f.poll(7070, SkatePose::riding).confirmed, "brief two-wheel contact does not blink the manual meter");
        check(!f.poll(7120, SkatePose::riding).confirmed, "sustained two-wheel native exit releases the old identity");
        f.context.contact = pose; f.context.activity_fresh = true;
        check(!f.poll(7130, pose, empty(1)).confirmed, "native contact cannot reuse the consumed label for a new manual");
    }
    check(manual_contact_pose({true, false, false, false, 100, 1, 0x8000}, false) == SkatePose::nose_manual &&
          manual_contact_pose({true, false, false, false, 100, 1, 0x4000}, false) == SkatePose::manual,
          "current native wheel contact independently retains each already-proved identity");
    check(manual_contact_pose({true, false, false, false, 100, 1, 0x8000}, true) == SkatePose::unknown &&
          manual_contact_pose({true, false, false, false, 201, 2, 0x8000}, false) == SkatePose::unknown,
          "native pushing and airborne state cannot masquerade as continuing manual contact");
    for (unsigned fault = 0; fault < 7; ++fault) {
        Fixture f; f.context.contact = SkatePose::nose_manual;
        f.poll(1010, SkatePose::nose_manual, edge(1010, 1, 2));
        if (fault == 0) f.context.valid = false;
        if (fault == 1) f.context.enabled = false;
        if (fault == 2) f.context.owner_changed = true;
        if (fault == 3) f.context.gap = true;
        if (fault == 4) ++f.context.generation;
        if (fault == 5) f.context.contact = SkatePose::unknown;
        if (fault == 6) f.context.contact = SkatePose::manual;
        check(!f.poll(1020, SkatePose::riding, empty(1)).confirmed,
              "native continuity cannot cross owner, menu, invalid state, observation gaps or opposite contact");
    }
    // HIL50 mixed01: generic activity plus one-wheel contact lasted64ms;
    // no matching Manual label. This replay asserts no fabricated native identity.
    {
        Fixture f; ComboSession combo(native_combo_tuning());
        combo.step(SkatePose::air, 0, 0);
        for (auto ms : {1010u, 1026u, 1042u, 1058u}) {
            const auto result = f.poll(ms, SkatePose::manual);
            check(result.pose == SkatePose::riding && !result.confirmed && !result.admitted,
                  "unconfirmed contact maps to riding without a manual link");
            combo.step(result.pose, .016f, 0);
        }
        combo.step(f.poll(1074, SkatePose::riding).pose, .016f, 0);
        check(combo.model().state().manual_fatigue == 0 && combo.model().state().balance_kind == BalanceKind::none,
              "64ms uncommanded candidate accrues no manual fatigue or balance");
        check(combo.model().state().fatigue < .12f && combo.model().state().link_grace_s < .75f,
              "contact candidate neither adds link fatigue nor renews the landing window");
    }
    // Real recorded label delays; timing is replayed, not claimed as new native acceptance.
    for (auto latency : {0u, 65u, 117u, 467u}) {
        Fixture f;
        f.poll(1010, SkatePose::manual);
        for (unsigned elapsed = 16; elapsed < latency; elapsed += 16)
            check(!f.poll(1010 + elapsed, SkatePose::manual).confirmed, "candidate waits for independent label");
        const auto accepted = f.poll(1010 + latency, SkatePose::manual, edge(1010 + latency));
        check(accepted.confirmed && accepted.admitted && accepted.pose == SkatePose::manual,
              "fresh matching label admits even a467ms-old continuous candidate");
        check(f.poll(1026 + latency, SkatePose::manual, empty(1)).confirmed,
              "continuous native contact retains already established identity");
    }
    {
        Fixture f;
        check(f.poll(1010, SkatePose::manual, edge(1010)).admitted, "fresh manual edge");
        check(f.poll(1020, SkatePose::manual, edge(1010)).confirmed &&
              !f.poll(1020, SkatePose::manual, edge(1010)).admitted, "exact repeated edge never awards another link");
        f.poll(1030, SkatePose::riding, empty(1));
        check(!f.poll(1040, SkatePose::manual, empty(1)).confirmed, "persistent same label cannot confirm a new section");
        check(!f.poll(1050, SkatePose::manual, edge(1010)).confirmed, "old edge cannot rearm after neutral contact");
        check(f.poll(1060, SkatePose::manual, edge(1060, 2)).admitted, "new observed occurrence can confirm repeated manual");
        check(!f.poll(1070, SkatePose::nose_manual, empty(2)).confirmed, "opposite wheel invalidates prior manual identity");
        check(f.poll(1080, SkatePose::nose_manual, edge(1080, 3, 2)).admitted, "nose manual requires its exact independent label");
    }
    {
        Fixture f;
        f.poll(1010, SkatePose::manual, edge(1010));
        for (auto ms : {1020u, 1070u, 1120u, 1170u, 1220u}) {
            auto snapshot = empty(1);
            snapshot.observed_at = 1010; snapshot.sampled_at = ms;
            const auto result = f.poll(ms, SkatePose::manual, snapshot);
            check(result.confirmed && !result.admitted,
                  "fresh empty rereads preserve continuous identity despite updated sampled timestamp");
        }
    }
    for (unsigned fault = 0; fault < 10; ++fault) {
        Fixture f;
        f.poll(1010, SkatePose::manual);
        auto label = edge(1020);
        if (fault == 0) label.valid = false;
        if (fault == 1) label.uncertainty = ScoreLabelUncertainty::coalesced;
        if (fault == 2) label.labels = score_label_bit(2);
        if (fault == 3) label.labels |= score_label_bit(2);
        if (fault == 4) label.observed_at = 1009;
        if (fault == 5) label.sampled_at = 1019;
        if (fault == 6) label.sampled_at = 1021;
        if (fault == 7) label.sequence = 0;
        if (fault == 8) label.sequence = 2;
        if (fault == 9) { label.observed_at = 100; label.sampled_at = 100; }
        check(!f.poll(1020, SkatePose::manual, label).confirmed, "invalid, stale, skipped or mismatched label grants no identity");
    }
    for (unsigned fault = 0; fault < 9; ++fault) {
        Fixture f;
        f.poll(1010, SkatePose::manual, edge(1010));
        if (fault == 0) f.context.valid = false;
        if (fault == 1) f.context.enabled = false;
        if (fault == 2) f.context.activity_fresh = false;
        if (fault == 3) f.context.owner_changed = true;
        if (fault == 4) f.context.gap = true;
        if (fault == 5) ++f.context.generation;
        if (fault == 6) f.context.generation = 0;
        if (fault == 7) f.gate.reset();
        const auto now = fault == 8 ? 1009u : 1020u;
        check(!f.poll(now, SkatePose::manual, edge(1010)).confirmed, "context change cannot retain or replay manual identity");
    }
    {
        Fixture f; f.poll(1010, SkatePose::manual, edge(1010));
        check(!f.poll(1161, SkatePose::manual, edge(1161, 2)).confirmed, "observer gap establishes baseline without admitting an edge");
        check(!f.poll(1170, SkatePose::manual, edge(1161, 2)).confirmed, "gap-consumed edge stays consumed");
        check(f.poll(1180, SkatePose::manual, edge(1180, 3)).admitted, "new evidence can recover after validated context resumes");
        check(!f.poll(1190, SkatePose::manual, edge(1010)).confirmed, "reordered edge invalidates confidence");
    }
    for (bool explicit_reset : {false, true}) {
        Fixture f; f.poll(1010, SkatePose::manual, edge(1010));
        const auto pending = edge(1020, 2);
        if (explicit_reset) f.gate.reset(pending);
        else {
            f.context.owner_changed = true;
            check(!f.poll(1020, SkatePose::manual, pending).confirmed,
                  "owner-change tick disqualifies its fresh pending edge");
            f.context.owner_changed = false;
        }
        ++f.context.generation;
        f.poll(1030, SkatePose::riding, empty(2));
        check(!f.poll(1040, SkatePose::manual, pending).confirmed,
              "fresh old-owner edge stays consumed after owner or explicit reset");
        check(f.poll(1050, SkatePose::manual, edge(1050, 3)).admitted,
              "next context requires a genuinely new observed label sequence");
    }
    {
        Fixture f;
        for (unsigned elapsed = 0; elapsed <= 3700; elapsed += 10)
            check(!f.poll(1010 + elapsed, SkatePose::manual).confirmed,
                  "sustained unlabelled one-wheel contact never promotes itself to manual identity");
    }
    {
        Fixture f; f.poll(1010, SkatePose::manual, edge(1010));
        check(!f.poll(1020, SkatePose::manual, edge(1020)).confirmed, "same serial with altered data is ambiguous");
        check(!f.poll(1030, SkatePose::manual, empty(1)).confirmed, "ambiguity cannot disappear on a persistent reread");
        check(f.poll(1040, SkatePose::manual, edge(1040, 2)).admitted, "fresh occurrence recovers identity");
        check(f.poll(1050, SkatePose::air, empty(2)).pose == SkatePose::air, "air remains independently observed air");
        check(f.poll(1060, SkatePose::bail, empty(2)).pose == SkatePose::bail, "bail is never hidden by manual filtering");
    }
    std::cout << checks << " manual identity observation checks passed\n";
}
