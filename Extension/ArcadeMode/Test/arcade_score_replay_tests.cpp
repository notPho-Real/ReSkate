#include "Extension/ArcadeMode/arcade_combo_session.h"
#include <cstdlib>
#include <iostream>

namespace {
using namespace dingosdk::arcade_mode;
using namespace dingosdk::arcade_mode::native_score_detail;
unsigned checks{};
void check(bool ok, const char* contract) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << contract << '\n'; std::exit(1); }
}
// Entirely synthetic adapter attestations. This corpus exercises admission and
// settlement contracts; it supplies no missing authority to production captures.
struct Replay {
    ComboSession session;
    ScoreContext context{11, 7, 3, 1000, 0, 0, 0, 0, true, true};
    ScoreProof proof{11, 7, 3, 1, true, true, true, ScoreMeaning::raw_action_points, 1};
    Event event{1, 1000, 7, Route::sequence_end, {100, 0, true}, true};
    Replay() {
        session.step(SkatePose::air, 0, 0);
        check(session.begin_score_evidence(context, 0), "independent synthetic combo baseline");
    }
    ScoreReceipt observe() { return session.observe_score(event, proof, context); }
    void next(ScoreMeaning meaning, double value, unsigned action = 0) {
        ++event.serial; ++proof.occurrence; context.now += 10;
        event.sampled_at = context.now; proof.meaning = meaning; proof.action = action;
        event.route = meaning == ScoreMeaning::raw_action_points ? Route::sequence_end : Route::line_end;
        event.payload = {value, 0, meaning == ScoreMeaning::raw_action_points};
    }
    void expire() {
        session.step(SkatePose::riding, .1f, 0);
        for (unsigned i = 0; i < 4; ++i) session.step(SkatePose::riding, .2f, 0);
    }
    void healthy_terminal(unsigned elapsed = 1000) {
        context.now += elapsed;
        check(session.refresh_score_evidence(context) == ScoreDecision::valid, "fresh complete end health");
    }
};
}

int run_arcade_score_replay_tests() {
    // A transport duplicate is harmless only when its authority proof agrees,
    // including assertions withdrawn by a later adapter read.
    for (unsigned mutation = 0; mutation < 9; ++mutation) {
        Replay r; check(r.observe().decision == ScoreDecision::valid, "initial award control");
        switch (mutation) {
        case 0: ++r.proof.owner; break;
        case 1: ++r.proof.generation; break;
        case 2: ++r.proof.combo; break;
        case 3: ++r.proof.occurrence; break;
        case 4: r.proof.local_owner = false; break;
        case 5: r.proof.native_identity = false; break;
        case 6: r.proof.semantics = false; break;
        case 7: r.proof.meaning = ScoreMeaning::bail; break;
        default: ++r.proof.action; break;
        }
        check(r.observe().decision == ScoreDecision::ambiguous && r.session.score_unverified() &&
              !r.session.model().state().raw_points,
              "contradictory authority on identical transport must invalidate credited points");
        r.expire(); check(!r.session.take_score_settlement(), "conflicted transport cannot settle later");
    }
    {
        Replay r; r.observe(); r.context.now += 100;
        check(r.observe().decision == ScoreDecision::duplicate, "unchanged transport and proof deduplicate");
        r.context.now -= 50;
        check(r.session.refresh_score_evidence(r.context) == ScoreDecision::stale &&
              r.session.score_unverified(), "duplicate delivery advances the observed context clock");
    }
    {
        Replay r; ScoreBoundary gate; gate.begin(r.context, 0);
        gate.observe(r.event, r.proof, r.context);
        auto lost = r.context; lost.health_complete = false;
        check(gate.refresh(lost) == ScoreDecision::incomplete, "health gap taints the boundary");
        check(gate.observe(r.event, r.proof, r.context).decision == ScoreDecision::incomplete,
              "transport replay after a health gap cannot claim harmless duplicate admission");
    }
    for (unsigned fault = 0; fault < 6; ++fault) {
        Replay r; r.observe(); r.session.step(SkatePose::manual, .01f, 0);
        const float fatigue = r.session.model().state().fatigue;
        switch (fault) {
        case 0: ++r.context.owner; break;
        case 1: ++r.context.generation; break;
        case 2: ++r.context.combo; break;
        case 3: ++r.context.dropped; break;
        case 4: ++r.context.rejected; break;
        default: r.context.health_complete = false; break;
        }
        check(r.session.refresh_score_evidence(r.context) != ScoreDecision::valid &&
              !r.session.model().state().raw_points && r.session.model().state().combo_active &&
              r.session.model().state().fatigue == fatigue,
              "owner generation combo or continuity loss removes only monetary authority");
        r.expire(); check(!r.session.take_score_settlement(), "lost authority cannot become a zero bank");
    }
    for (bool reverse_awards : {false, true}) {
        Replay r;
        r.session.step(SkatePose::manual, .01f, 0);
        r.session.step(SkatePose::grind, .01f, 0);
        r.session.step(SkatePose::grind_exit_transition, .01f, 0);
        r.session.step(SkatePose::air, .01f, 0);
        check(r.session.model().state().multiplier == 2, "physical transitions determine action slots");
        r.proof.action = reverse_awards ? 2u : 1u;
        r.event.payload.score = reverse_awards ? 30 : 70;
        check(r.observe().decision == ScoreDecision::valid, "verified action attribution may arrive in either order");
        check(r.observe().decision == ScoreDecision::duplicate && r.session.model().state().raw_points ==
              (reverse_awards ? 30u : 70u), "exact award replay cannot double points");
        r.next(ScoreMeaning::raw_action_points, reverse_awards ? 70 : 30, reverse_awards ? 1u : 2u);
        check(r.observe().decision == ScoreDecision::valid && !r.session.score_unverified() &&
              r.session.model().state().raw_points == 100, "both observed actions covered once");
        r.session.step(SkatePose::riding, .01f, 0);
        r.session.step(SkatePose::riding, .01f, 0, true);
        r.session.step(SkatePose::nose_manual, .01f, 0);
        check(r.session.model().state().combo_active && r.session.model().state().raw_points == 100 &&
              r.session.model().state().multiplier == 2, "manual grind exit and revert preserve credited attribution");
        r.next(ScoreMeaning::clean_end, 9999); r.observe();
        r.expire(); check(!r.session.take_score_settlement(), "model expiry needs later stream health");
        r.healthy_terminal(); const auto bank = r.session.take_score_settlement();
        check(bank && bank->kind == SettlementKind::bank && bank->points == 200 && bank->actions == 2,
              "raw awards multiply once; unrelated native aggregate is never added");
        check(!r.session.take_score_settlement(), "settlement drains once");
    }
    for (unsigned fault = 0; fault < 5; ++fault) {
        Replay r; r.observe(); r.next(ScoreMeaning::raw_action_points, 100, 1);
        if (fault == 0) r.proof.occurrence = 1, r.event.payload.score = 101;
        if (fault == 1) ++r.event.serial;
        if (fault == 2) r.event.sampled_at = 999;
        if (fault == 3) r.event.payload.score = 1.5;
        if (fault == 4) r.proof.semantics = false; // An aggregate cannot masquerade as raw points.
        check(r.observe().decision != ScoreDecision::valid && r.session.score_unverified() &&
              !r.session.model().state().raw_points, "conflicting identity gaps time and unknown value semantics fail closed");
    }
    for (bool bail_first : {false, true}) {
        Replay r; r.observe();
        r.next(bail_first ? ScoreMeaning::bail : ScoreMeaning::clean_end, 100); r.observe();
        r.next(bail_first ? ScoreMeaning::clean_end : ScoreMeaning::bail, 100);
        check(r.observe().decision != ScoreDecision::valid, "conflicting terminal order is not another settlement");
        const auto prior = r.session.take_score_settlement();
        check(bail_first ? (prior && prior->kind == SettlementKind::bail && prior->points == 100) : !prior,
              "published loss is immutable; unconfirmed clean end cannot survive conflicting bail");
        check(!r.session.take_score_settlement(), "terminal conflict never duplicates a receipt");
    }
    {
        Replay r; r.observe(); r.next(ScoreMeaning::clean_end, 100); r.observe();
        r.expire(); r.healthy_terminal(100);
        r.proof.semantics = false;
        check(r.observe().decision == ScoreDecision::ambiguous,
              "changed proof on a fresh terminal replay reaches authority comparison");
        const auto bank = r.session.take_score_settlement();
        check(bank && bank->kind == SettlementKind::bank && bank->points == 100 &&
              !r.session.take_score_settlement(),
              "later proof invalidation cannot overwrite an independently published undrained bank");
    }
    {
        Replay r; r.observe(); r.session.invalidate();
        r.session.step(SkatePose::air, 0, 0); ++r.context.owner; ++r.context.combo;
        r.proof.owner = r.context.owner; r.proof.combo = r.context.combo;
        check(r.session.begin_score_evidence(r.context, r.event.serial), "independent owner/combo establishes new namespace");
        ++r.event.serial; r.event.sampled_at = ++r.context.now;
        check(r.observe().decision == ScoreDecision::valid, "occurrence value may repeat only within independently new context");
    }
    std::cout << checks << " adversarial score replay checks passed\n";
    return 0;
}
#ifndef ARCADE_SCORE_REPLAY_COMBINED_TESTS
int main() { return run_arcade_score_replay_tests(); }
#endif
