#include "Extension/ArcadeMode/arcade_combo_session.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
using namespace dingosdk::arcade_mode;
using namespace dingosdk::arcade_mode::native_score_detail;
unsigned checks{};
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << label << '\n'; std::exit(1); }
}
// Synthetic proof fixtures exercise the boundary contract only. No existing
// native capture has these ownership/identity/point-semantics attestations.
struct Fixture {
    ScoreContext context{11, 7, 3, 1000, 0, 0, 0, 0, true, true};
    ScoreProof proof{11, 7, 3, 1, true, true, true, ScoreMeaning::raw_action_points, 1};
    Event event{1, 1000, 7, Route::sequence_end, {100, 0, true}, true};
    void next(ScoreMeaning meaning = ScoreMeaning::raw_action_points, double points = 100) {
        ++event.serial; ++proof.occurrence; context.now += 10; event.sampled_at = context.now;
        proof.meaning = meaning;
        proof.action = meaning == ScoreMeaning::raw_action_points ? proof.action + 1 : 0;
        event.route = meaning == ScoreMeaning::raw_action_points ? Route::sequence_end : Route::line_end;
        event.payload = {points, 0, meaning == ScoreMeaning::raw_action_points};
    }
};
void expire(ComboSession& session) {
    for (int i = 0; i < 3; ++i) session.step(SkatePose::riding, .2f, 0);
}
}

int run_arcade_score_settlement_tests() {
    {
        Fixture f;
        check(native_score_capture_status(f.event, 7, 1000, true) == ScoreDecision::ambiguous,
              "raw capture is never authority, even with its diagnostic flag set");
        check(native_score_capture_status(f.event, 8, 1000, true) == ScoreDecision::stale, "retired generation");
        check(native_score_capture_status(f.event, 7, 1151, true) == ScoreDecision::stale, "delayed worker drain");
        check(native_score_capture_status(f.event, 7, 999, true) == ScoreDecision::stale, "clock reversal");
        f.event.payload.reason = 7;
        check(native_score_capture_status(f.event, 7, 1000, true) == ScoreDecision::invalid,
              "sequence cannot invent line-end reason");
    }
    {
        Fixture f; ScoreBoundary gate; gate.begin(f.context, 0);
        check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::valid, "complete proof admitted");
        check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::duplicate, "exact transport replay");
        f.next(); f.proof.occurrence = 1; f.proof.action = 1;
        check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::duplicate, "native duplicate across captures");
        f.next(); f.proof.occurrence = 2;
        check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::valid, "equal points do not imply duplicate");
        f.next(ScoreMeaning::clean_end, 200); f.proof.occurrence = 3;
        check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::valid, "independent clean finalization");
        f.next(ScoreMeaning::clean_end, 200); f.proof.occurrence = 3; f.event.route = Route::stats_line_end;
        check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::duplicate,
              "second end listener with same independently established identity deduplicates");
        f.next(ScoreMeaning::clean_end, 201); f.proof.occurrence = 3;
        check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::ambiguous && gate.tainted(),
              "conflicting duplicate is never silently accepted");
    }
    for (unsigned failure = 0; failure < 18; ++failure) {
        Fixture f; ScoreBoundary gate; gate.begin(f.context, 0);
        auto expected = ScoreDecision::ambiguous;
        switch (failure) {
        case 0: f.event.owner_verified = false; break;
        case 1: f.proof.local_owner = false; break;
        case 2: f.proof.native_identity = false; break;
        case 3: f.proof.semantics = false; break;
        case 4: f.proof.occurrence = 0; break;
        case 5: ++f.proof.owner; break;
        case 6: ++f.proof.combo; break;
        case 7: f.event.payload.score = 1.5; break;
        case 8: f.event.payload.success = false; break;
        case 9: f.context.now += 151; expected = ScoreDecision::stale; break;
        case 10: ++f.context.generation; expected = ScoreDecision::stale; break;
        case 11: ++f.context.owner; expected = ScoreDecision::stale; break;
        case 12: f.event.serial = 2; expected = ScoreDecision::incomplete; break;
        case 13: ++f.context.dropped; expected = ScoreDecision::incomplete; break;
        case 14: ++f.context.rejected; expected = ScoreDecision::incomplete; break;
        case 15: ++f.context.contended; expected = ScoreDecision::incomplete; break;
        case 16: ++f.context.stale; expected = ScoreDecision::incomplete; break;
        default: f.context.health_complete = false; expected = ScoreDecision::incomplete; break;
        }
        check(gate.observe(f.event, f.proof, f.context).decision == expected && gate.tainted(),
              "uncertain or incomplete evidence closes authority");
    }
    {
        Fixture f; ScoreBoundary gate; gate.begin(f.context, 0);
        for (std::size_t i = 0; i < ScoreBoundary::capacity; ++i) {
            check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::valid, "bounded journal entry");
            f.next();
        }
        check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::incomplete,
              "capacity fails closed instead of evicting deduplication history");
        ++f.context.combo; f.proof.combo = f.context.combo;
        gate.begin(f.context, f.event.serial - 1);
        check(gate.observe(f.event, f.proof, f.context).decision == ScoreDecision::valid,
              "independently delimited new combo resets journal and capture clock");
    }
    {
        Fixture f; ComboSession session;
        session.step(SkatePose::air, 0, 0);
        check(session.begin_score_evidence(f.context, 0), "begin attributable active combo");
        check(session.observe_score(f.event, f.proof, f.context).decision == ScoreDecision::valid, "credit first action");
        check(session.model().state().raw_points == 100 && session.model().state().multiplier == 1,
              "native points do not count the observed takeoff twice");
        session.step(SkatePose::grind, .1f, 0);
        session.step(SkatePose::manual, .1f, 0);
        const auto fatigue = session.model().state().fatigue;
        check(session.observe_score(f.event, f.proof, f.context).decision == ScoreDecision::duplicate &&
              session.model().state().balance_kind == BalanceKind::manual &&
              session.model().state().fatigue == fatigue, "duplicate receipt preserves live balance and fatigue");
        session.step(SkatePose::air, .1f, 0); f.next(ScoreMeaning::raw_action_points, 50);
        check(session.observe_score(f.event, f.proof, f.context).decision == ScoreDecision::valid &&
              session.model().state().raw_points == 150 && session.model().state().multiplier == 2, "linked score follows existing multiplier");
        session.step(SkatePose::riding, .1f, 0);
        f.next(ScoreMeaning::clean_end, 999);
        check(session.observe_score(f.event, f.proof, f.context).decision == ScoreDecision::valid &&
              !session.take_score_settlement(), "native line total is not added or banked before Arcade grace expires");
        expire(session);
        auto bank = session.take_score_settlement();
        check(bank && bank->kind == SettlementKind::bank && bank->points == 300,
              "clean completion banks the model amount once, not the native aggregate");
        check(!session.take_score_settlement() && !session.model().state().combo_active &&
              session.model().state().fatigue == 0 && session.model().state().grind_fatigue == 0 &&
              session.model().state().manual_fatigue == 0, "bank resets fatigue and drains once");
    }
    {
        Fixture f; ComboSession session;
        session.step(SkatePose::air, 0, 0); session.begin_score_evidence(f.context, 0);
        session.observe_score(f.event, f.proof, f.context);
        session.step(SkatePose::riding, .1f, 0); expire(session);
        check(!session.take_score_settlement(), "model expiry without independent clean end stays pending");
        f.next(ScoreMeaning::clean_end, 100);
        session.observe_score(f.event, f.proof, f.context);
        check(session.take_score_settlement().has_value(), "fresh finalization can confirm an already completed model bank");
    }
    {
        Fixture f; ComboSession session;
        session.step(SkatePose::air, 0, 0); session.begin_score_evidence(f.context, 0);
        session.observe_score(f.event, f.proof, f.context);
        session.step(SkatePose::grind, .1f, 0); session.step(SkatePose::bail, .1f, 0);
        const auto serial = session.model().last_settlement().sequence;
        for (int i = 0; i < 20; ++i) session.step(SkatePose::bail, .02f, 0);
        check(session.model().last_settlement().sequence == serial && !session.take_score_settlement(),
              "persistent bail does not manufacture new outcomes or prove monetary loss");
        f.next(ScoreMeaning::bail, 100); f.event.payload.reason = 7;
        session.observe_score(f.event, f.proof, f.context);
        const auto loss = session.take_score_settlement();
        check(loss && loss->kind == SettlementKind::bail && loss->points == 100 &&
              session.model().state().fatigue == 0 && session.model().state().raw_points == 0,
              "independently attributed bail loses unbanked amount and resets fatigue");
        session.step(SkatePose::riding, .1f, 0); session.step(SkatePose::air, .1f, 0);
        ++f.context.combo; f.proof.combo = f.context.combo;
        check(session.begin_score_evidence(f.context, f.event.serial), "fresh combo works after confirmed bail");
        f.next(); session.observe_score(f.event, f.proof, f.context);
        check(session.model().state().raw_points == 100 && session.model().state().multiplier == 1,
              "fresh combo has independent points and multiplier");
    }
    {
        Fixture f; ComboSession session;
        session.step(SkatePose::air, 0, 0); session.begin_score_evidence(f.context, 0);
        session.observe_score(f.event, f.proof, f.context);
        session.step(SkatePose::manual, .1f, 0);
        const auto fatigue = session.model().state().fatigue;
        f.next(); f.context.dropped = 1;
        check(session.observe_score(f.event, f.proof, f.context).decision == ScoreDecision::incomplete &&
              session.score_unverified() && session.model().state().raw_points == 0 &&
              session.model().state().fatigue == fatigue && session.model().state().combo_active,
              "loss removes monetary authority without faking a physical bail or resetting fatigue");
        session.step(SkatePose::riding, .1f, 0); expire(session);
        check(!session.take_score_settlement(), "incomplete evidence never turns into a zero-point bank");
    }
    {
        Fixture f; ComboSession session;
        session.step(SkatePose::air, 0, 0); session.begin_score_evidence(f.context, 0);
        check(session.score_unverified(), "baseline alone does not verify score");
        f.proof.meaning = ScoreMeaning::clean_end; f.event.route = Route::line_end;
        f.event.payload = {0, 0, false};
        session.observe_score(f.event, f.proof, f.context);
        session.step(SkatePose::riding, .1f, 0); expire(session);
        check(!session.take_score_settlement() && session.score_unverified(),
              "missing action points plus clean end is not proof of zero banking");
    }
    {
        Fixture f; ComboSession session;
        session.step(SkatePose::air, 0, 0); session.begin_score_evidence(f.context, 0);
        session.observe_score(f.event, f.proof, f.context);
        session.step(SkatePose::manual, .1f, 0); session.step(SkatePose::air, .1f, 0);
        f.next(ScoreMeaning::clean_end, 100); session.observe_score(f.event, f.proof, f.context);
        session.step(SkatePose::riding, .1f, 0); expire(session);
        check(session.model().last_settlement().points == 200 && !session.take_score_settlement() &&
              session.score_unverified(), "missing second action cannot silently contribute zero points to an authoritative bank");
    }
    {
        Fixture f; ComboSession session;
        session.step(SkatePose::air, 0, 0); session.begin_score_evidence(f.context, 0);
        session.observe_score(f.event, f.proof, f.context);
        f.next(); f.proof.action = 1;
        check(session.observe_score(f.event, f.proof, f.context).decision == ScoreDecision::ambiguous &&
              session.model().state().raw_points == 0, "two different occurrences cannot claim one observed action slot");
    }
    {
        Fixture f; ComboSession session;
        session.step(SkatePose::air, 0, 0); session.begin_score_evidence(f.context, 0);
        f.event.payload.score = 0;
        session.observe_score(f.event, f.proof, f.context);
        f.next(ScoreMeaning::clean_end, 0); session.observe_score(f.event, f.proof, f.context);
        session.step(SkatePose::riding, .1f, 0); expire(session);
        const auto zero = session.take_score_settlement();
        check(zero && zero->points == 0 && zero->kind == SettlementKind::bank,
              "explicitly verified zero points remain distinct from missing evidence");
    }
    {
        Fixture f; ComboSession session;
        session.step(SkatePose::air, 0, 0); session.begin_score_evidence(f.context, 0);
        session.observe_score(f.event, f.proof, f.context);
        session.step(SkatePose::riding, .1f, 0);
        session.step(SkatePose::riding, .1f, 0, true);
        const auto multiplier = session.model().state().multiplier;
        for (int i = 0; i < 5; ++i) session.step(SkatePose::riding, .1f, 0, true);
        check(!session.model().state().combo_active && !session.take_score_settlement() && multiplier == 1,
              "repeated revert does not farm multiplier or keep scored combo alive");
    }
    std::cout << checks << " score admission and settlement checks passed\n";
    return 0;
}
#ifndef ARCADE_SCORE_SETTLEMENT_COMBINED_TESTS
int main() { return run_arcade_score_settlement_tests(); }
#endif
