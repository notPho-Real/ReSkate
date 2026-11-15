#include "Extension/ArcadeMode/arcade_combo_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk;
using namespace dingosdk::arcade_mode;
void check(bool ok, const char* why) { if (!ok) { std::cerr << why << '\n'; std::exit(1); } }
tricks::NativeScoreSequence sequence(unsigned i, unsigned points, std::initializer_list<std::uint64_t> actions) {
    tricks::NativeScoreSequence value; value.index=i; value.points=points; value.action_objects=actions; return value;
}
int main() {
    tricks::NativeScoreSnapshot s; s.hud=1; s.scoring=2; s.manager=3;
    tricks::NativeScoreProgress p;
    check(!p.observe(s,true,0), "joining a running combo cannot credit old records");
    p.reset(); check(p.observe(s,false,0).has_value(), "idle owner establishes a baseline");
    s.current.current_action=100;
    auto value=p.observe(s,true,0);
    check(value && !value->actions && !value->points, "selected action is not a committed trick");
    s.current=sequence(0,15,{100}); value=p.observe(s,true,0);
    check(value && value->actions==1 && value->points==15, "committed action counts once");
    for(unsigned i=0;i<120;++i) check(p.observe(s,true,0)->actions==1, "held action never counts per frame");
    s.current=sequence(0,20,{200}); value=p.observe(s,true,0);
    check(value && value->actions==2 && value->points==20, "two committed actions count separately even when they reuse the same native slot");
    s.current=sequence(0,35,{200,300}); value=p.observe(s,true,0);
    check(value && value->actions==3 && value->points==35, "native appended action adds a multiplier step");
    s.previous=sequence(0,30,{200,300}); s.completed={s.previous}; value=p.observe(s,true,0);
    check(value && value->actions==3 && value->points==30, "landing correction wins over an unfinalized copy");
    s.current=sequence(1,10,{400}); value=p.observe(s,true,0);
    check(value && value->actions==4 && value->points==40, "native copies deduplicate across sequence transitions");
    s.previous_line=s.completed; s.completed.clear(); value=p.observe(s,true,0);
    check(value && value->points==40, "historical copy does not double count");
    // The Deck's Manual -> Ollie trace reuses index zero in both native
    // history entries. An empty live handoff distinguishes the two sequences.
    s.current={};s.completed={sequence(0,30,{200,300}),sequence(1,10,{400})};p.observe(s,true,0);
    s.current=sequence(0,9,{500});value=p.observe(s,true,0);
    check(value && value->actions==5 && value->points==49, "reused sequence number still admits the next performed trick");
    s.current={};s.completed.push_back(sequence(0,9,{500}));value=p.observe(s,true,0);
    check(value && value->actions==5 && value->points==49, "distinct native history positions cannot collapse into one index");
    check(p.observe(s,false,1).has_value(), "next idle baseline recovers");
    check(p.observe(s,true,1)->actions==0, "lingering history cannot enter the next combo");
    s.current=sequence(3,14,{700}); check(p.observe(s,true,1)->actions==1, "fresh next-combo native entry counts");
    ++s.manager; check(!p.observe(s,true,1), "registry replacement invalidates native totals");
    p.observe(s,false,2); p.observe(s,true,2);
    check(!p.observe(std::nullopt,true,2) && !p.observe(s,true,2), "missing observation cannot silently resume scoring");
    p.observe(s,false,3); s.current=sequence(4,5,{0});
    check(!p.observe(s,true,3), "null action is not an occurrence");
    p.reset(); s.current={}; s.previous={}; s.previous_line.clear(); s.completed.clear();
    ComboSession session({},true);
    check(session.observe_native_score(s), "production session establishes idle baseline");
    session.step(SkatePose::manual,0,0);
    check(session.model().state().multiplier==0, "physical Manual entry alone does not mint a multiplier");
    s.current=sequence(0,10,{100}); check(session.observe_native_score(s), "native Manual action admitted");
    const auto before=session.model().state();
    const NativeInputOwner owner{1,2,3,4,5};
    check(session.observe_custom({owner,1,1,1,1,1000,ArcadeTrick::casper,true,0},owner,1,1000), "custom proof remains independent");
    s.current.points=20; check(session.observe_native_score(s), "native held points replace subtotal");
    check(session.model().state().multiplier==2 && session.model().state().raw_points==20 &&
        session.model().state().custom_points==250 && session.model().state().balance==before.balance,
        "native revision preserves custom credit and physical pose");
    session.step(SkatePose::air,0,0); check(session.model().state().multiplier==2, "takeoff is lifecycle only");
    s.current=sequence(1,15,{200}); s.completed={sequence(0,20,{100})}; session.observe_native_score(s);
    check(session.model().state().multiplier==3 && session.model().state().raw_points==35, "next performed trick adds one");
    session.step(SkatePose::riding,0,0); session.step(SkatePose::riding,.25f,0); session.step(SkatePose::riding,.25f,0);
    check(session.model().last_settlement().points==855, "combo banks native plus custom points times performed actions");
    session.observe_native_score(s); session.step(SkatePose::manual,0,0);
    s.current=sequence(2,10,{300}); session.observe_native_score(s); session.step(SkatePose::bail,0,0);
    check(!session.model().state().raw_points && !session.model().state().multiplier &&
        session.model().last_settlement().kind==SettlementKind::bail, "bail loses native and custom provisional totals");
    tricks::NativeScoreSnapshot rapid;rapid.hud=1;rapid.scoring=2;rapid.manager=3;
    tricks::NativeScoreProgress skipped;skipped.observe(rapid,false,0);
    rapid.current=sequence(0,10,{10});skipped.observe(rapid,true,0);
    rapid.previous=sequence(0,8,{10});rapid.completed={rapid.previous};rapid.current=sequence(0,20,{20});
    const auto handoff=skipped.observe(rapid,true,0);
    check(handoff && handoff->actions==2 && handoff->points==28,
        "a skipped empty frame cannot double-count the prior Ollie in the following Heelflip");
    rapid.previous=sequence(0,18,{20});rapid.completed.push_back(rapid.previous);
    const auto finalized=skipped.observe(rapid,true,0);
    check(finalized && finalized->actions==2 && finalized->points==26,
        "finalized live and history copies remain one performed action each");
    tricks::NativeScoreProgress missed;rapid={1,2,3,{},{},{},{}};missed.observe(rapid,false,0);
    rapid.current=sequence(0,15,{30});rapid.previous=rapid.current;rapid.completed={rapid.current};
    check(missed.observe(rapid,true,0)->actions==1,
        "first observation of an already finalized action counts once across live and history copies");
    // Captured device case: the animation runs Ollie, then Heelflip, but
    // sequence_actions first commits only Heelflip. Points remain native.
    using tricks::NativeScoreMotion;
    tricks::NativeScoreSnapshot flight_score{1,2,3,{},{},{},{}};
    tricks::NativeScoreProgress flight_progress;
    NativeScoreMotion ground{7,0,false,0}, pop{7,1,true,1}, heel{7,1,true,3};
    flight_progress.observe(flight_score,false,0,ground);
    auto flight_value=flight_progress.observe(flight_score,true,0,pop);
    check(flight_value && flight_value->actions==1 && !flight_value->points,
        "performed Ollie appears before native scoring commits the follow-up");
    for(unsigned i=0;i<120;++i)check(flight_progress.observe(flight_score,true,0,pop)->actions==1,
        "held Ollie animation contributes once per physical flight");
    flight_score.current.current_action=77;
    check(flight_progress.observe(flight_score,true,0,heel)->actions==1,
        "selected Heelflip without committed action earns nothing");
    flight_score.current=sequence(0,75,{77});
    flight_value=flight_progress.observe(flight_score,true,0,heel);
    check(flight_value && flight_value->actions==2 && flight_value->points==75,
        "device Ollie then Heelflip counts two while preserving the native points");
    flight_score.previous=flight_score.current;flight_score.completed={flight_score.current};flight_score.current={};
    ground.flight=1;flight_progress.observe(flight_score,true,0,ground);
    pop.flight=2;flight_progress.observe(flight_score,true,0,pop);
    flight_score.current=sequence(0,2,{88});flight_score.current.ollie_actions={88};
    flight_value=flight_progress.observe(flight_score,true,0,pop);
    check(flight_value && flight_value->actions==3 && flight_value->points==77,
        "a later committed Ollie reconciles only its own flight, preserving the earlier replaced Ollie");
    flight_score.previous=flight_score.current;flight_score.completed.push_back(flight_score.current);flight_score.current={};
    ground.flight=2;flight_progress.observe(flight_score,true,0,ground);
    pop.flight=3;flight_progress.observe(flight_score,true,0,pop);
    flight_score.current=sequence(0,3,{99});flight_score.current.ollie_actions={99};
    flight_value=flight_progress.observe(flight_score,true,0,pop);
    check(flight_value && flight_value->actions==4,"consecutive standalone Ollies do not double count");
    heel.flight=3;flight_score.current=sequence(0,80,{100});
    check(flight_progress.observe(flight_score,true,0,heel)->actions==5,
        "Ollie that was already committed before replacement still counts exactly once");
    flight_score.current=sequence(0,95,{100,101});
    check(flight_progress.observe(flight_score,true,0,heel)->actions==6,
        "repeated committed flips remain additive alongside the observed Ollie");
    check(!flight_progress.observe(flight_score,true,0,std::nullopt),"lost motion ownership invalidates combined scoring");
    flight_progress.reset();flight_score={1,2,3,{},{},{},{}};
    ground={7,0,false,0};pop={7,1,true,1};heel={7,1,true,3};
    flight_progress.observe(flight_score,false,0,ground);
    flight_progress.observe(flight_score,true,0,pop);
    ground.flight=1;flight_score.completed={sequence(0,2,{200})};flight_score.completed[0].ollie_actions={200};
    flight_value=flight_progress.observe(flight_score,true,0,ground);
    check(flight_value && flight_value->actions==1,"Ollie first committed at landing reconciles its observed flight");
    flight_progress.reset();flight_score={1,2,3,{},{},{},{}};
    ground.flight=0;flight_progress.observe(flight_score,false,0,ground);
    flight_score.current=sequence(0,75,{300});
    check(flight_progress.observe(flight_score,true,0,heel)->actions==1,
        "flip without observed Ollie does not invent a pop");
    heel.owner=8;check(!flight_progress.observe(flight_score,true,0,heel),"physics owner replacement invalidates active score");
    flight_progress.reset();flight_score={1,2,3,{},{},{},{}};
    flight_progress.observe(flight_score,false,0,pop);
    check(flight_progress.observe(flight_score,true,0,pop)->actions==0,"joining mid-flight cannot credit a lingering Ollie");
    flight_progress.reset();flight_progress.observe(flight_score,false,0,ground);
    NativeScoreMotion no_animation{7,1,true,-1};
    check(flight_progress.observe(flight_score,true,0,no_animation)->actions==0,"physical takeoff without animation proof is not a trick");
    std::cout << "Native action slots, revisions, ownership, settlement and balance preservation passed\n";
}
