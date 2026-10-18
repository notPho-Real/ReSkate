#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>

namespace {
using namespace dingosdk::arcade_mode;
unsigned checks{};
void check(bool ok, const char* contract) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << contract << '\n'; std::exit(1); }
}
ScoreLabelOccurrence empty(std::uint64_t sequence) {
    return {true, 0, sequence, 0, 0, ScoreLabelUncertainty::none};
}
NativeComboObservation native(SkatePose pose, bool active = true) {
    switch (pose) {
    case SkatePose::air: return {true, true, active, false, 200, 0, 0};
    case SkatePose::grind: return {true, true, active, false, 403, 1, 0};
    case SkatePose::grind_exit_transition: return {true, true, active, false, 701, 1, 0};
    case SkatePose::manual: return {true, true, active, false, 100, 1, 0x4000};
    case SkatePose::nose_manual: return {true, true, active, false, 100, 1, 0x8000};
    case SkatePose::riding: return {true, true, active, false, 100, 1, 0xc000};
    case SkatePose::bail: return {true, true, active, true, 300, 1, 0};
    default: return {};
    }
}
struct Trace {
    ComboSession combo{native_combo_tuning()};
    ManualObservation manual;
    std::uint64_t now{1000}, sequence{};
    ScoreLabelOccurrence last_identity{};
    Trace() { sample(SkatePose::riding, 0); }
    void sample(SkatePose physical, unsigned elapsed, bool fresh_identity = false,
                bool activity = true, bool revert = false, const ScoreLabelOccurrence* replay = nullptr) {
        now += elapsed;
        const auto raw = native(physical, activity);
        auto label = empty(sequence);
        if (fresh_identity) {
            ++sequence;
            label = {true, score_label_bit(physical == SkatePose::nose_manual ? 2u : 1u),
                sequence, now, now, ScoreLabelUncertainty::none};
            last_identity = label;
        }
        if (replay) label = *replay;
        if (!raw.valid) combo.invalidate(); // Production owner/state loss precedes pose adaptation.
        const auto admitted = manual.poll(observed_pose(raw), label,
            {now, 7, true, raw.valid && !raw.bailed, activity, false, false,
             manual_contact_pose(raw, false)});
        combo.step(admitted.pose, static_cast<float>(elapsed) / 1000.0f, 0, revert);
    }
    void segment(SkatePose pose, unsigned duration, unsigned frame, bool identity = false) {
        sample(pose, 0, identity);
        for (unsigned elapsed = 0; elapsed < duration;) {
            const unsigned part = std::min(frame, duration - elapsed);
            sample(pose, part); elapsed += part;
            const auto before = combo.model().state();
            sample(pose, 0); // Exact duplicate observation, not a second action or new time.
            const auto after = combo.model().state();
            check(before.multiplier == after.multiplier && before.fatigue == after.fatigue &&
                  before.grind_fatigue == after.grind_fatigue && before.manual_fatigue == after.manual_fatigue &&
                  before.balance == after.balance && before.balance_velocity == after.balance_velocity,
                  "duplicate native samples cannot farm actions or alter persistent fatigue/dynamics");
        }
    }
    void end(unsigned frame) {
        sample(SkatePose::riding, 0);
        // Admitted manual contact can retain its established pose for100ms
        // before the unchanged750ms grounded landing window begins.
        segment(SkatePose::riding, 1000, frame);
        const auto& s = combo.model().state();
        check(!s.combo_active && !s.fatigue && !s.manual_fatigue && !s.grind_fatigue,
              "observed grounded expiry resets all fatigue exactly once");
        const auto serial = combo.model().last_settlement().sequence;
        segment(SkatePose::riding, 100, frame);
        check(combo.model().last_settlement().sequence == serial,
              "duplicate grounded terminal samples cannot create more outcomes");
        check(!combo.take_score_settlement(), "physical chain tracing cannot invent native monetary authority");
    }
};
struct Phase { SkatePose pose; unsigned ms; };
// Model-level observation projections of A-F. Multiple named airborne tricks
// cannot be inferred from pose alone; these traces claim continuity only.
constexpr std::array a{Phase{SkatePose::air, 120}, Phase{SkatePose::grind, 120}};
constexpr std::array c{Phase{SkatePose::air, 120}, Phase{SkatePose::manual, 120}, Phase{SkatePose::air, 120}};
constexpr std::array d{Phase{SkatePose::grind, 120}, Phase{SkatePose::grind_exit_transition, 52}, Phase{SkatePose::air, 120}};
constexpr std::array e{Phase{SkatePose::grind, 120}, Phase{SkatePose::grind_exit_transition, 52},
    Phase{SkatePose::air, 120}, Phase{SkatePose::manual, 120}};
constexpr std::array f{Phase{SkatePose::grind, 120}, Phase{SkatePose::grind_exit_transition, 52},
    Phase{SkatePose::air, 120}, Phase{SkatePose::manual, 120}, Phase{SkatePose::air, 120},
    Phase{SkatePose::grind, 120}, Phase{SkatePose::grind_exit_transition, 52},
    Phase{SkatePose::air, 120}, Phase{SkatePose::nose_manual, 120}};
}

int run_arcade_combo_trace_tests() {
    for (unsigned frame : {17u, 34u}) {
        for (const std::span<const Phase> phases : {std::span<const Phase>(a), std::span<const Phase>(c),
                std::span<const Phase>(d), std::span<const Phase>(e), std::span<const Phase>(f)}) {
            Trace t; State previous{};
            std::array<float, 2> last_difficulty{-1.f, -1.f};
            for (const auto& phase : phases) {
                t.segment(phase.pose, phase.ms, frame, balance_kind(phase.pose) == BalanceKind::manual);
                const auto& s = t.combo.model().state();
                check(s.combo_active && s.fatigue >= previous.fatigue &&
                      s.grind_fatigue >= previous.grind_fatigue && s.manual_fatigue >= previous.manual_fatigue,
                      "interleaved rail/air/Manual/Nose sections retain all fatigue histories");
                previous = s;
                const auto kind = balance_kind(phase.pose);
                if (kind != BalanceKind::none) {
                    const unsigned index = kind == BalanceKind::grind ? 0u : 1u;
                    check(t.combo.model().difficulty() > last_difficulty[index],
                          "later sections of the same balance family are harder within the same chain");
                    last_difficulty[index] = t.combo.model().difficulty();
                }
            }
            t.end(frame);
        }
        // B: accepted landing -> native revert -> independently admitted manual.
        for (SkatePose pose : {SkatePose::manual, SkatePose::nose_manual}) {
            Trace t; t.segment(SkatePose::air, 120, frame); t.sample(SkatePose::riding, 0);
            t.segment(SkatePose::riding, 600, frame);
            t.sample(SkatePose::riding, 0, false, true, true);
            t.segment(SkatePose::riding, 950, frame);
            const auto before = t.combo.model().state();
            t.sample(pose, 0, true);
            check(t.combo.model().state().combo_active && !t.combo.model().state().revert_pending &&
                  t.combo.model().state().fatigue >= before.fatigue,
                  "native-tuned 600ms revert plus 950ms manual admission consumes one bounded link");
            t.segment(pose, 120, frame); t.end(frame);
        }
        // Recontact and repeated Manual/Nose sections, including score-display dropouts.
        Trace t;
        for (unsigned repetition = 0; repetition < 3; ++repetition) {
            const auto before = t.combo.model().state();
            t.segment(SkatePose::grind, 100, frame);
            t.segment(SkatePose::grind_exit_transition, 52, frame);
            t.segment(SkatePose::grind, 100, frame);
            t.segment(SkatePose::air, 100, frame);
            t.segment(SkatePose::manual, 100, frame, true);
            for (unsigned i = 0; i < 3; ++i) t.sample(SkatePose::manual, frame, false, false);
            check(t.combo.model().state().balance_kind == BalanceKind::manual,
                  "fresh same-owner contact retains admitted manual across score activity dropout");
            t.segment(SkatePose::air, 100, frame);
            t.segment(SkatePose::nose_manual, 100, frame, true);
            check(t.combo.model().state().grind_fatigue > before.grind_fatigue &&
                  t.combo.model().state().manual_fatigue > before.manual_fatigue,
                  "repeated rail recontacts and both manual identities increase mode fatigue");
        }
        t.end(frame);
    }
    // G: repeated independently observed reverts never refresh one accepted deadline.
    {
        ComboSession combo(native_combo_tuning()); combo.step(SkatePose::air, 0, 0);
        combo.step(SkatePose::riding, 0, 0);
        for (unsigned i = 0; i < 3; ++i) combo.step(SkatePose::riding, .2f, 0);
        combo.step(SkatePose::riding, 0, 0, true);
        for (unsigned i = 0; i < 10; ++i) combo.step(SkatePose::riding, .1f, 0, true);
        check(combo.model().state().combo_active && combo.model().state().multiplier == 1,
              "repeated revert observations neither farm action count nor bank before deadline");
        combo.step(SkatePose::riding, .1f, 0, true);
        check(!combo.model().state().combo_active, "revert expires at unchanged 1100ms despite duplicates");
    }
    // H and dropout ordering: no artificial continuation through a lost owner/state.
    for (SkatePose stop : {SkatePose::unknown, SkatePose::bail}) {
        Trace t; t.segment(SkatePose::air, 100, 17); t.segment(SkatePose::manual, 100, 17, true);
        t.sample(stop, 17);
        check(!t.combo.model().state().combo_active && !t.combo.model().state().fatigue,
              "unknown observation or physical bail resets continuity rather than preserving guessed fatigue");
        const auto serial = t.combo.model().last_settlement().sequence;
        t.sample(stop, 17);
        check(t.combo.model().last_settlement().sequence == serial, "repeated loss/bail is idempotent");
        t.segment(SkatePose::riding, 100, 17); t.segment(SkatePose::air, 100, 17);
        check(t.combo.model().state().combo_active && !t.combo.model().state().manual_fatigue &&
              !t.combo.model().state().grind_fatigue, "fresh recovery starts without prior mode fatigue");
    }
    for (SkatePose pose : {SkatePose::manual, SkatePose::nose_manual}) {
        Trace t; t.segment(SkatePose::air, 100, 17);
        t.segment(pose, 100, 17); // Physical contact without a new matching history edge.
        check(t.combo.model().state().balance_kind == BalanceKind::none &&
              !t.combo.model().state().manual_fatigue, "unconfirmed wheel contact cannot grant a manual continuation");
    }
    for (bool expired : {false, true}) {
        ComboSession combo(native_combo_tuning());
        combo.step(SkatePose::manual, 0, 0); combo.step(SkatePose::manual, .1f, 0);
        combo.step(SkatePose::air, 0, 0); combo.step(SkatePose::riding, 0, 0);
        const auto before = combo.model().state();
        combo.step(SkatePose::riding, .25f, 0); combo.step(SkatePose::riding, .25f, 0);
        combo.step(SkatePose::riding, expired ? .25f : .24f, 0);
        const auto outcome = combo.model().last_settlement();
        combo.step(SkatePose::nose_manual, 0, 0);
        check(expired ? (outcome.kind == SettlementKind::bank && !combo.model().state().manual_fatigue) :
                       (outcome.kind == SettlementKind::none &&
                        combo.model().state().manual_fatigue == before.manual_fatigue &&
                        combo.model().state().fatigue >= before.fatigue),
              "740ms continuation preserves fatigue while750ms expiry starts an independent section");
    }
    for (bool bail : {false, true}) {
        ComboSession combo(native_combo_tuning()); combo.step(SkatePose::air, 0, 0);
        combo.step(SkatePose::riding, 0, 0);
        combo.step(SkatePose::riding, .25f, 0); combo.step(SkatePose::riding, .25f, 0);
        combo.step(SkatePose::riding, .24f, 0);
        combo.step(bail ? SkatePose::bail : SkatePose::riding, .02f, 0);
        check(combo.model().last_settlement().kind == (bail ? SettlementKind::bail : SettlementKind::bank),
              "observed bail wins over expiry in the same poll; ordinary expiry remains a model bank");
    }
    {
        Trace t; t.segment(SkatePose::manual, 100, 17, true);
        const auto fatigue = t.combo.model().state().manual_fatigue;
        t.sample(SkatePose::nose_manual, 17);
        check(t.combo.model().state().balance_kind == BalanceKind::none &&
              t.combo.model().state().manual_fatigue >= fatigue,
              "opposite wheel contact without its new identity does not inherit manual admission");
        t.sample(SkatePose::nose_manual, 17, true);
        check(t.combo.model().state().balance_kind == BalanceKind::manual &&
              t.combo.model().state().manual_fatigue >= fatigue,
              "new independently matching Nose identity resumes balance without erasing mode history");
        const auto consumed_nose = t.last_identity;
        t.combo.invalidate(); t.manual.reset(); // OFF/context-reset contract, no input module involved.
        check(!t.combo.model().state().combo_active && !t.combo.model().state().manual_fatigue,
              "OFF/context invalidation resets all persistent fatigue");
        t.sample(SkatePose::riding, 0);
        t.sample(SkatePose::nose_manual, 17, false, true, false, &consumed_nose);
        check(consumed_nose.valid && consumed_nose.labels == score_label_bit(2) &&
              !t.combo.model().state().combo_active,
              "reenabling cannot reuse an old consumed native label to restore a section");
    }
    std::cout << checks << " interleaved combo trace checks passed\n";
    return 0;
}
#ifndef ARCADE_COMBO_TRACE_COMBINED_TESTS
int main() { return run_arcade_combo_trace_tests(); }
#endif
