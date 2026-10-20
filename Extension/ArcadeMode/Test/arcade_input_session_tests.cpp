#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_native_events.h"
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* reason) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << reason << '\n'; std::exit(1); }
}
std::vector<PreparedGesture> catalog() {
    std::vector<PreparedGesture> result;
    for (const unsigned id : {1u, 2u, 3u, 4u, 16u}) {
        const bool nollie = id == 16;
        result.emplace_back(static_cast<std::uint8_t>(id), nollie, GestureContext::ground,
            std::vector<AuthoredStickStep>{{0, 0, static_cast<std::int16_t>(nollie ? 32767 : -32767)},
                {18, 25000, 28000}, {21, 0, 0}, {24, 0, 0}});
    }
    return result;
}
struct Harness {
    InputSession session;
    std::vector<PreparedGesture> prepared{catalog()};
    InputAdmission admission{};
    Harness() {
        admission.gameplay = admission.authored = admission.manual_safe = admission.action_fresh = true;
        admission.manual_balance_admitted = true;
        admission.may_start = admission.inspected_stance = true;
        admission.action_mode = ActionExperiment::classic;
        admission.pose = SkatePose::riding; admission.catalog = prepared;
    }
    NativeGesture::Output poll(std::uint64_t now, PhysicalInputSample physical = {}) {
        admission.tick = now / 17;
        return session.poll(now, physical, 0, 0, admission);
    }
};
void native_adapters() {
    Harness h;
    h.poll(0);
    auto out = h.poll(17, {0x1000});
    check(out.authored_playback && out.requested_id == 1 && out.right_y == -32767, "shared A edge starts unchanged native crouch setup");
    check(h.poll(170, {0x1000}).right_y == -32767, "held A retains authored setup");
    out = h.poll(204);
    check(out.authored_playback, "release uses existing native playback timeline");
    h.session.reset(); h.poll(300); h.admission.pose = SkatePose::air;
    out = h.poll(317, {0x4000, -25000, 0});
    check(out.requested_id == 2 && out.authored_playback, "airborne flip uses exact existing native mapping and prepared path");
    h.admission.pose = SkatePose::riding;
    check(!h.poll(334).authored_playback, "landing cancels airborne authored ownership");
    h.session.reset(); h.admission.pose = SkatePose::air; h.poll(400);
    out = h.poll(417, {0x2000, 25000, 0});
    check(out.action.requested && out.action.owned && out.action.trick == ArcadeTrick::basic_bs_grab &&
        out.action.pad.right_trigger == 255 && !out.action.pad.owns_left_stick, "basic grab adapter owns only measured channels with actual identity");
    check(h.poll(916, {0x2000}).action.pad.right_trigger == 255, "basic grab remains held before the former fixed duration");
    check(h.poll(917, {0x2000}).action.pad.right_trigger == 255, "basic grab holds beyond old fixed500ms duration");
    check(h.poll(934).action.pad.right_trigger == 0, "physical release begins explicit native neutral");
    check(!h.poll(968).action.owned, "basic grab releases ownership after its34ms neutral tail");
    check(!h.poll(985).action.requested, "held B cannot replay after native recipe completes");
    h.admission.pose = SkatePose::riding;
    check(h.poll(1002, {0x2000}).buttons == 0x2000, "grounded B remains physical native brake");
    h.session.reset(); h.poll(1000);
    out = h.poll(1017, {0, 0, 0, 0, 255});
    check(out.action.owned && out.action.pad.right_y == -32767 && out.action.request_kind == 3,
        "fresh RT reaches the unchanged native revert recipe");
    check(h.poll(1047, {0, 0, 0, 0, 255}).action.pad.left_x > 32000, "revert rotation follows existing absolute-time recipe");
    check(!h.poll(1252, {0, 0, 0, 0, 255}).action.owned, "revert releases at235ms without renewed grace");
    h.session.reset(); h.poll(1300); h.admission.nollie = true;
    out = h.poll(1317, {0, 0, 0, 255, 255});
    check(!out.action.owned && out.action.result == DispatchResult::unsafe_context, "unverified revert stance cannot own sticks");
    h.admission.nollie = false; h.session.reset(); h.poll(1400);
    h.poll(1417, {0x8001}); h.admission.pose = SkatePose::air;
    out = h.poll(1451, {0x8001});
    check(out.phase == 1 && out.right_y == 20000 && !out.authored_playback, "held ground-to-air Y retains measured partial board tilt");
    h.admission.pose = SkatePose::grind;
    check(h.poll(1485, {0x8001}).right_y == 20000, "native rail engagement preserves held alignment");
    check(h.poll(1519).phase == 0, "Y release restores neutral synthetic alignment");
    h.session.reset(); h.admission.pose = SkatePose::air; h.poll(1600);
    out = h.poll(1617, {0x8004});
    check(out.unsupported && !out.phase, "unverified lateral rail selection never substitutes a partial vertical method");
}
void one_journal_and_boundaries() {
    Harness h; h.poll(0);
    h.poll(17, {0, 0, 25000}); h.poll(34, {0, 0, -25000});
    check(h.session.history().events().size() == 3, "manual uses the session's single journal");
    h.admission.pose = SkatePose::air; h.poll(51, {0x4000, -25000, 0});
    h.poll(68, {0x2000}); h.poll(85, {0x8000}); h.poll(102, {0, 0, 0, 0, 255});
    bool button{}, vertical{}, trigger{};
    for (const auto& event : h.session.history().events()) {
        button = button || event.token.source == InputSource::buttons;
        vertical = vertical || event.token.source == InputSource::direction;
        trigger = trigger || event.token.source == InputSource::triggers;
    }
    check(button && vertical && trigger && InputHistory::capacity == 100, "all action families share one100-entry event/consumption domain");
    for (unsigned boundary = 0; boundary < 5; ++boundary) {
        Harness test; test.poll(0); test.poll(17, {0x1000});
        if (boundary == 0) test.admission.gameplay = false;
        if (boundary == 1) test.admission.offboard = true;
        if (boundary == 2) test.admission.pose = SkatePose::bail;
        if (boundary == 3) test.session.reset();
        auto out = test.poll(boundary == 4 ? 16 : 34, {0x1000});
        check(!out.authored_playback && !out.action.owned && !out.phase, "hard boundary cancels all native owners");
        test.admission.gameplay = true; test.admission.offboard = false; test.admission.pose = SkatePose::riding;
        out = test.poll(100, {0x1000});
        check(!out.authored_playback && !out.requested_id, "held-through-boundary A cannot become a fresh press");
    }
    ArcadeInput input; input.enable(true);
    input.poll(0, 0, 0, 0, 0, 0, 0, true, true);
    input.enable(false);
    const auto off = input.poll(0, 17, 0xf30f, -30000, 20000, 12000, -14000, true, true);
    check(off.buttons == 0xf30f && off.right_x == 12000 && off.right_y == -14000 && !off.phase,
        "Arcade OFF restores all physical output channels without a second recognizer");
    check(native_combo_tuning().combo_link_grace_s == .75f && native_combo_tuning().revert_link_grace_s == 1.1f,
        "runtime750/1100ms combo tuning remains independent of input dispatch");
}
void unsupported_branches() {
    for (unsigned variant = 0; variant < 8; ++variant) {
        constexpr std::array first{14u, 14u, 13u, 13u, 13u, 15u, 15u, 15u};
        constexpr std::array second{13u, 15u, 14u, 15u, 13u, 15u, 14u, 13u};
        Harness h; h.admission.pose = SkatePose::manual; h.poll(0);
        h.poll(17, {static_cast<std::uint16_t>(1u << first[variant])}); h.poll(34);
        const auto out = h.poll(85, {static_cast<std::uint16_t>(1u << second[variant])});
        check(out.unsupported && !out.phase && !out.action.owned && !out.requested_id,
            "every unsupported contextual branch emits no substitute native recipe");
        const auto& diagnostic = h.session.diagnostic();
        check(diagnostic.dispatched && diagnostic.dispatch[0].disposition == InputDisposition::unsupported &&
            diagnostic.dispatch[0].request.variant == variant + 1, "branch selection remains separate from performed native identity");
    }
    Harness h; h.admission.pose = SkatePose::manual; h.admission.observed_balance_branch = 2; h.poll(0);
    h.poll(17, {0x8000}); h.poll(34); const auto out = h.poll(85, {0x8000});
    check(out.unsupported && h.session.diagnostic().dispatch[0].request.variant == 6,
        "observer-supplied branch context evaluates the next generic branch without inventing native IDs");
}
void balance_transition_contracts() {
    constexpr std::array first{14u, 14u, 13u, 13u, 13u, 15u, 15u, 15u};
    constexpr std::array second{13u, 15u, 14u, 15u, 13u, 15u, 14u, 13u};
    constexpr std::array target{13u, 0u, 0u, 14u, 0u, 0u, 255u, 0u};
    for (unsigned i = 0; i < first.size(); ++i) {
        Harness h; h.admission.pose = SkatePose::manual; h.admission.observed_balance_branch = i + 1; h.poll(0);
        h.poll(17, {static_cast<std::uint16_t>(1u << first[i])}); h.poll(34);
        const auto out = h.poll(85, {static_cast<std::uint16_t>(1u << second[i])});
        const auto& diagnostic = h.session.diagnostic();
        check((out.unsupported || (target[i] == 255 && diagnostic.unresolved_definition)) && !out.phase && !out.manual.owns && !out.requested_id,
            "excluded current branch preserves unsupported or unresolved execution without native pose fabrication");
        check(target[i] == 255 ? diagnostic.dispatched == 0 : diagnostic.dispatched == 1 && diagnostic.dispatch[0].request.variant == target[i],
            "duplicate fallback rows retain their original table and exclusion order");
        const auto recognized = diagnostic.recognized;
        h.poll(102);
        check(h.session.diagnostic().recognized == recognized && h.admission.observed_balance_branch == i + 1,
            "return consumption does not repeat or advance independently supplied native context");
    }
    Harness excluded; excluded.admission.pose = SkatePose::manual; excluded.admission.observed_balance_branch = 9;
    excluded.poll(0); excluded.poll(17, {4}); excluded.poll(34); excluded.poll(51, {8});
    check(!excluded.poll(85, {0x4000}).unsupported,
        "current triple target is excluded before matching without consuming its candidate events");

    for (unsigned mode = 0; mode < 4; ++mode) {
        Harness h; h.poll(0);
        if (mode == 1) h.admission.pose = SkatePose::air;
        h.poll(17, {0x4000, mode == 1 ? std::int16_t{-25000} : std::int16_t{0}, 0});
        if (mode == 2) { h.admission.gameplay = false; h.poll(34); h.admission.gameplay = true; }
        h.admission.pose = SkatePose::manual;
        const auto out = h.poll(mode == 3 ? 317 : 85, {0x8000});
        check(out.unsupported == (mode == 0),
            "admitted balance preserves eligible prefix but rejects consumed, hard-reset and expired history");
        if (mode == 0) check(h.session.diagnostic().dispatch[0].request.variant == 2,
            "unconsumed pre-contact X joins post-contact Y in the shared journal");
        if (mode == 1) {
            bool used{};
            for (const auto& event : h.session.history().events())
                if (event.token.source == InputSource::buttons && event.token.code == 14 && event.token.edge == InputEdge::press)
                    used = event.owners == 1;
            check(used, "balance entry preserves existing category ownership instead of clearing or restoring events");
        }
    }
}
void manual_readiness_contracts() {
    const auto setup = [](Harness& h, bool airborne) {
        h.session.set_schedule(AuthoredPlaybackSchedule::absolute_ticks);
        h.poll(0); h.poll(17, {0x1000}); h.poll(34);
        if (airborne) h.admission.pose = SkatePose::air;
        h.admission.tick = 2;
        h.session.poll(51, {0, 0, 25000}, 0, 0, h.admission);
        const auto out = h.session.poll(68, {0, 0, -25000}, 0, 0, h.admission);
        check(out.authored_playback && !out.manual.owns && !out.manual.request,
            "safe manual recognition waits behind native execution without owning any manual output");
        const auto pending = h.session.pending_manual();
        check(pending.action == InputAction::manual && pending.recognized_at == 68 &&
            pending.unbounded == !airborne && (pending.unbounded || pending.deadline == 768),
            "production exposes safe pending recognition independently of native actuation ownership");
    };
    for (bool airborne : {false, true}) {
        Harness h; setup(h, airborne);
        h.admission.tick = 5;
        h.session.poll(85, {}, 0, 0, h.admission); // Deliver the existing authored neutral.
        const auto out = h.session.poll(102, {}, 0, 0, h.admission);
        check(out.manual.owns && out.right_y == -20000 && !out.authored_playback &&
            out.manual.request_at == (airborne ? 68u : 102u),
            "safely deferred manual starts only after existing owner delivers neutral, preserving its proper clock");
    }
    Harness ground; setup(ground, false);
    ground.session.poll(1000, {}, 0, 0, ground.admission);
    ground.admission.tick = 5; ground.session.poll(1017, {}, 0, 0, ground.admission);
    check(ground.session.poll(1034, {}, 0, 0, ground.admission).manual.owns,
        "ground pending selection has no invented one-millisecond expiry while context stays safe");
    Harness air; setup(air, true);
    air.admission.tick = 5; air.session.poll(766, {}, 0, 0, air.admission);
    auto out = air.session.poll(767, {}, 0, 0, air.admission);
    check(out.manual.owns && out.manual.request_at == 68, "air readiness at699ms retains original recognition timestamp");
    check(air.session.poll(768, {}, 0, 0, air.admission).manual.owns &&
          !air.session.poll(769, {}, 0, 0, air.admission).manual.owns,
        "deferred native air actuation includes700ms and expires afterward without renewal");
    Harness expired; setup(expired, true);
    expired.session.poll(769, {}, 0, 0, expired.admission);
    check(expired.session.diagnostic().dispatched == 1 && expired.session.diagnostic().dispatch[0].disposition == InputDisposition::expired,
        "native inclusive air deadline expires explicitly even while execution is busy");
    expired.admission.tick = 5; expired.session.poll(785, {}, 0, 0, expired.admission);
    check(!expired.session.poll(802, {}, 0, 0, expired.admission).manual.owns,
        "expired readiness cannot execute after the owner becomes available");
    Harness landing; setup(landing, true); landing.admission.pose = SkatePose::riding;
    landing.admission.tick = 5; landing.session.poll(85, {}, 0, 0, landing.admission);
    out = landing.session.poll(102, {}, 0, 0, landing.admission);
    check(out.manual.owns && out.manual.request_at == 102, "safe delayed landing begins the separate native settling clock");
    landing.admission.pose = SkatePose::manual;
    check(landing.session.poll(119, {}, 0, 0, landing.admission).manual.confirmed,
        "delayed landing still requires independently supplied native contact confirmation");
    for (unsigned boundary = 0; boundary < 8; ++boundary) {
        Harness h; setup(h, true);
        if (boundary == 0) h.admission.manual_safe = false;
        if (boundary == 1) h.admission.external_owner = true;
        if (boundary == 2) h.admission.gameplay = false;
        if (boundary == 3) h.admission.pose = SkatePose::grind;
        if (boundary == 4) h.admission.pose = SkatePose::bail;
        if (boundary == 5) h.admission.pose = SkatePose::unknown;
        const PhysicalInputSample interruption = boundary == 6 ? PhysicalInputSample{0x2000} :
            boundary == 7 ? PhysicalInputSample{0, 0, 0, 255, 0} : PhysicalInputSample{};
        h.session.poll(85, interruption, 0, 0, h.admission);
        h.admission.manual_safe = h.admission.gameplay = true; h.admission.external_owner = false;
        h.admission.pose = SkatePose::air; h.admission.tick = 5;
        h.session.poll(102, {}, 0, 0, h.admission);
        check(!h.session.poll(119, {}, 0, 0, h.admission).manual.owns,
            "unsafe or hard-boundary cancellation never revives safely buffered input");
    }
}

void unsupported_contexts() {
    Harness missing; missing.admission.authored = false; missing.poll(0);
    auto out = missing.poll(17, {0x1000});
    check(out.unsupported && !out.phase && !out.requested_id,
        "missing authored catalog cannot synthesize an unverified fallback jump");
    Harness lab; lab.admission.action_mode = ActionExperiment::grabs; lab.admission.pose = SkatePose::air; lab.poll(0);
    lab.poll(17, {0x2004}); lab.poll(34, {4}); lab.poll(85, {0x2004});
    check(!lab.poll(264, {4}).action.requested, "explicit lab selector defers repeated named selection without ownership");
    out = lab.poll(265, {4});
    check(out.action.requested && out.action.trick == ArcadeTrick::method && out.action.owned,
        "explicit lab selector retains its existing named recipe adapter after shared recognition");
    Harness lip; lip.admission.observed_lip = true; lip.poll(0);
    out = lip.poll(17, {0x8000});
    check(out.unsupported && !out.phase && !out.action.owned &&
        lip.session.diagnostic().dispatch[0].request.action == InputAction::lip_request,
        "independently supplied lip context recognizes request but cannot fabricate native execution");
    for (unsigned branch : {0u, 2u, 6u}) {
        Harness h; h.admission.pose = SkatePose::manual; h.admission.observed_balance_branch = branch; h.poll(0);
        h.poll(17, {0x4000}); h.poll(34); out = h.poll(85, {0x4000});
        check(out.unsupported && h.session.diagnostic().dispatch[0].request.variant == (branch == 0 ? 22 : branch == 2 ? 42 : 51),
            "posture extra repeated press precedes generic branch matching");
        Harness left; left.admission.pose = SkatePose::manual; left.admission.observed_balance_branch = branch; left.poll(0);
        out = left.poll(17, {0, 0, 0, 255, 0});
        check(out.unsupported == (branch != 0), "left spin extra requires an eligible observed branch context");
    }
}

void jump_release_metadata() {
    for (unsigned duration : {0u,5u,50u,199u,200u,250u}) for (bool nollie : {false,true}) for (bool rail : {false,true}) {
        Harness h; h.admission.nollie=nollie;
        if (rail) { h.admission.pose=SkatePose::grind; for(auto& gesture:h.prepared) gesture.context=GestureContext::from_grind; }
        h.poll(0); h.poll(10,{0x1000}); h.admission.nollie=!nollie;
        const auto out=h.poll(10+duration);
        check(out.jump.released && out.jump.setup_admitted && out.jump.nollie==nollie && out.jump.from_rail==rail,
            "release metadata preserves the admitted setup context and press stance");
        check(out.jump.pressed_at==10 && out.jump.released_at==10+duration && out.jump.charge_ms()==(duration>200?200:duration),
            "charge uses physical timestamps with a fixed200ms ceiling");
        check(!h.poll(300+duration).jump.released,"one physical release cannot manufacture a second launch token");
    }
    Harness lip;lip.admission.observed_lip=true;lip.poll(0);const auto no_lip_actor=lip.poll(10,{0x1000});
    check(no_lip_actor.unsupported && !no_lip_actor.phase && !no_lip_actor.authored_playback && !no_lip_actor.requested_id,
        "lip exit cannot substitute ordinary grounded authored setup without an admitted lip actor");
    Harness absent; absent.admission.authored=false;absent.poll(0);absent.poll(10,{0x1000});
    const auto out=absent.poll(60);
    check(out.jump.released && !out.jump.setup_admitted && !out.authored_playback && !out.requested_id,
        "missing setup assets preserve recognition without fabricating native launch admission");
    Harness blocked;blocked.poll(0);blocked.poll(10,{0x1000});blocked.admission.pose=SkatePose::air;blocked.admission.flip_ready=false;
    blocked.poll(20,{0x5000,-25000,0});const auto released=blocked.poll(30,{0x4000,-25000,0});
    check(released.jump.released && released.jump.setup_admitted && released.jump.charge_ms()==20,
        "a waiting flip cannot swallow physical release of an admitted setup");
    check(!blocked.poll(40).jump.released,"imperative release acknowledgment removes the blocked queued release");
}

void repeated_air_safety() {
    for(unsigned step:{17u,34u}) {
        InputSession session;InputAdmission a{};const auto prepared=catalog();
        a.gameplay=a.authored=a.action_fresh=a.inspected_stance=a.may_start=true;
        a.action_mode=ActionExperiment::classic;a.pose=SkatePose::air;a.catalog=prepared;
        unsigned dispatched{};
        const std::array<unsigned,3> times=step==17?std::array{153u,357u,561u}:std::array{170u,510u,578u};
        for(unsigned now=0;now<1000;now+=step) {
            a.tick=now/step;
            const auto buttons=static_cast<std::uint16_t>(((now>=150&&now<200)||(now>=350&&now<400)||(now>=550&&now<600))?0x4000:0);
            const auto out=session.poll(now,{buttons,static_cast<std::int16_t>(now>=100?-25000:0),0},0,0,a);
            const auto& d=session.diagnostic();
            for(std::size_t i=0;i<d.dispatched;++i) {
                const auto& event=d.dispatch[i];check(dispatched<times.size() && now==times[dispatched],
                    "identical physical traces preserve recognition separately from authored readiness at both cadences");
                const bool extra=dispatched==1;
                check(event.request.action==InputAction::flip && event.request.presses==(extra?2u:1u) &&
                    event.request.direction==4 && event.request.direction_inherited==extra,
                    "only an admitted parent extra inherits direction; fresh held-direction actions remain ordinary");
                check(extra ? event.disposition==InputDisposition::unsupported && out.unsupported && !out.requested_id :
                    event.disposition==InputDisposition::started && out.requested_id==2,
                    "unsupported repetition never substitutes a native identity or advances its parent table");
                ++dispatched;
            }
        }
        check(dispatched==3,"every physical air action has exactly one terminal dispatch disposition");
    }
}

void airborne_setup_eligibility() {
    Harness h;h.admission.pose=SkatePose::air;h.poll(0);
    const auto press=h.poll(10,{0x1000});
    check(!press.authored_playback && !press.requested_id && !press.phase,
        "a lingering ground admission deadline cannot start a grounded setup after observed takeoff");
    check(!h.poll(50,{0x1000}).authored_playback,"held airborne input cannot create a grounded setup later");
    const auto release=h.poll(80);
    check(release.jump.released && !release.jump.setup_admitted && !release.authored_playback,
        "physical release remains observable but cannot claim a native setup that was never admitted");
    h.admission.pose=SkatePose::riding;check(h.poll(100,{0x1000}).authored_playback,
        "a fresh grounded physical press can start the existing native setup");
}

void queued_grab_identity() {
    // F07/F10/I10: immutable recognized identity is distinct from live readiness.
    for (unsigned step : {17u,34u}) for (bool recognized_nollie : {false,true}) {
        Harness h; h.admission.pose=SkatePose::air;
        const auto poll=[&](std::uint64_t now,PhysicalInputSample physical) {
            h.admission.native_context={true,true,{1,1,1},now+1,now,false,true,1,0};
            return h.poll(now,physical);
        };
        poll(0,{});
        check(poll(step,{0x4000,-25000,0}).authored_playback,"queued Grab fixture has an existing Flip owner");
        h.admission.nollie=recognized_nollie;
        poll(2*step,{0x2000,-25000,0});
        check(h.session.pending_grab().action==InputAction::grab && h.session.pending_grab().nollie==recognized_nollie,
            "queued Grab preserves its recognized launch family");
        h.admission.nollie=!recognized_nollie;
        unsigned terminal{};
        for (unsigned now=3*step;now<1200;now+=step) {
            const auto out=poll(now,{0x2000,-25000,0});
            check(!out.action.owned,"a later readiness change cannot substitute basic Grab for the recognized unsupported identity");
            const auto& diagnostic=h.session.diagnostic();
            for(std::size_t i=0;i<diagnostic.dispatched;++i) if(diagnostic.dispatch[i].request.action==InputAction::grab) {
                const auto& event=diagnostic.dispatch[i];
                check(event.request.nollie==recognized_nollie && event.request.recognized_at==2*step &&
                    event.disposition==InputDisposition::unsupported,"queued Grab ends explicitly unsupported without identity or timestamp renewal");
                ++terminal;
            }
        }
        check(terminal==1 && h.session.pending_grab().action==InputAction::none,"held Grab cannot replay the unsupported queued request");
    }
}

void manual_exit_jump_ownership() {
    // D09/B01/B03/I13/I15: exercise the existing facade across actor handoff,
    // rather than infer a native hop from a separate release-metadata fixture.
    for(unsigned step:{17u,34u})for(bool nose:{false,true})for(bool pad:{false,true}) {
        Harness h;
        const auto physical=[&](std::uint16_t face,int sign) {
            return PhysicalInputSample{static_cast<std::uint16_t>(face | (pad?(sign>0?1u:sign<0?2u:0u):0u)),0,
                static_cast<std::int16_t>(pad?0:sign*25000)};
        };
        const int first=nose?-1:1;
        h.poll(0);h.poll(step,physical(0,first));h.poll(2*step);
        const auto entered=h.poll(3*step,physical(0,-first));
        check(entered.manual.owns && entered.right_y==(nose?20000:-20000),"human reversal enters the selected Manual/Nose owner");
        h.admission.pose=nose?SkatePose::nose_manual:SkatePose::manual;
        h.admission.nollie=nose;
        check(h.poll(4*step,physical(0,-first)).manual.owns,"independently admitted balance retains its owner");
        const auto setup=h.poll(5*step,physical(0x1000,-first));
        check(!setup.manual.owns && setup.authored_playback && setup.requested_id==(nose?16:1),"fresh A transfers input ownership from balance to the captured setup family");
        h.admission.nollie=!nose;
        const auto release=h.poll(6*step,physical(0,-first));
        check(release.jump.released && release.jump.setup_admitted && release.jump.nollie==nose &&
            release.jump.pressed_at==5*step && release.jump.released_at==6*step,"balance exit keeps physical release and original setup identity");
        h.admission.pose=SkatePose::air;
        for(unsigned now=7*step;now<30*step;now+=step) {
            const auto held=h.poll(now,physical(0,-first));
            check(!held.jump.released && !held.manual.owns,"held exit direction cannot recreate balance or replay the jump release");
        }
    }
}

// Production Session composition: these fixtures supply existing adapter data
// and fresh observations; requested IDs do not establish native occurrence.
using ConformanceRecord = void(*)(const char*,std::uint64_t,const NativeGesture::Output&,
    const UnifiedInput::Diagnostic&,const InputHistory&);
ConformanceRecord conformance_record{};
struct ConformanceSession {
    InputSession session; InputAdmission admission{};
    std::vector<PreparedGesture> prepared;
    std::uint64_t step{}, episode{1}; bool air{true}; const char* trace{"air-query"};
    ConformanceSession() {
        for (const auto& row : literal_trick_table) if (row.family==TrickFamily::flip && row.presses==1) {
            const auto id=native_flip_id(row.trick,false);
            prepared.emplace_back(id,false,GestureContext::ground,
                std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}});
        }
        prepared.emplace_back(1,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}});
        admission.gameplay=admission.authored=admission.action_fresh=admission.inspected_stance=true;
        admission.may_start=true;admission.action_mode=ActionExperiment::classic;admission.pose=SkatePose::air;
        admission.catalog=prepared;session.set_recognition_capture(true);
    }
    NativeGesture::Output poll(std::uint64_t now,PhysicalInputSample physical={}) {
        const bool next_air=admission.pose==SkatePose::air;
        if(next_air&&!air)++episode;air=next_air;
        admission.tick=now/17;
        admission.native_context={true,true,{1,1,1},++step,now,false,air,episode,episode};
        const auto out=session.poll(now,physical,0,0,admission);
        if(conformance_record)conformance_record(trace,now,out,session.diagnostic(),session.history());
        return out;
    }
};
PhysicalInputSample conformance_direction(std::int16_t code,bool pad,std::uint16_t face=0) {
    std::uint16_t bits{};
    if(code==4||code==1||code==7)bits|=4;
    if(code==6||code==3||code==9)bits|=8;
    if(code==8||code==7||code==9)bits|=1;
    if(code==2||code==1||code==3)bits|=2;
    return {static_cast<std::uint16_t>(face|(pad?bits:0)),
        static_cast<std::int16_t>(pad?0:bits&4u?-25000:bits&8u?25000:0),
        static_cast<std::int16_t>(pad?0:bits&1u?25000:bits&2u?-25000:0)};
}
void production_air_query_matrix() {
    for(bool pad:{false,true})for(auto code:{7,9,1,3,8,2,4,6})
    for(bool held:{false,true})for(bool unrelated:{false,true})for(auto age:{499u,500u,501u}) {
        ConformanceSession h;h.trace=held?"held-direction":"released-direction";
        h.poll(0);h.poll(1,conformance_direction(static_cast<std::int16_t>(code),pad));
        if(!held)h.poll(2);
        if(unrelated)h.poll(3,conformance_direction(static_cast<std::int16_t>(held?code:0),pad,0x0100));
        const auto out=h.poll(age+1,conformance_direction(static_cast<std::int16_t>(held?code:0),pad,0x4000));
        const bool expected=held||(!unrelated&&age<500);
        const auto& d=h.session.diagnostic();
        check(d.recognition_count==(expected?1u:0u),
            "actual Session applies held/released direction windows and the press-only intervening barrier for every ordinary flip direction");
        if(expected) {
            const auto& request=d.recognition[0].request;
            const auto id=native_flip_id(air_selection(TrickFamily::flip,static_cast<std::int16_t>(code),1).trick,false);
            check(request.form==InputForm::ordinary && request.direction==code && request.presses==1 &&
                request.physical_pressed_at==age+1 && out.requested_id==id && out.authored_playback,
                "source row selection and existing native request ID remain distinct from native completion");
            bool consumed{};for(const auto& event:h.session.history().events())
                if(event.token==InputToken{InputSource::buttons,14,InputEdge::press})consumed=event.owners==InputConsumption::regular;
            check(consumed,"only successful ordinary Session matching commits its face category");
        } else check(!out.requested_id&&!out.authored_playback,"failed query creates no native request or owner");
    }
    for(auto age:{499u,500u,501u})for(bool released:{false,true}) {
        ConformanceSession h;h.trace="face-before-direction";h.poll(0);h.poll(1,{0x4000});
        if(released)h.poll(2);
        const auto out=h.poll(age+1,{static_cast<std::uint16_t>(released?0:0x4000),-25000});
        check((out.requested_id==2)==(age<500),"held-direction query times the actual buffered face even after face release");
    }
    for(auto code:{4,6,8,2})for(auto age:{299u,300u,301u})for(bool pad:{false,true}) {
        ConformanceSession h;h.trace="directional-triple";
        h.poll(0);h.poll(10,conformance_direction(static_cast<std::int16_t>(code),pad));h.poll(20);
        h.poll(30,conformance_direction(static_cast<std::int16_t>(code),pad));
        const auto out=h.poll(age+10,conformance_direction(static_cast<std::int16_t>(code),pad,0x4000));
        const auto& d=h.session.diagnostic();check(d.recognition_count==1,"triple boundary always resolves exactly one installed air row");
        check((d.recognition[0].request.form==InputForm::repeated_direction)==(age<300),
            "strict300ms directional triple precedes its independently eligible ordinary fallback");
        check(age<300?out.unsupported&&!out.authored_playback:out.authored_playback,
            "unpublished directional-prefix actor remains unsupported while an ordinary native request can run");
    }
    for(auto cadence:{17u,34u})for(unsigned phase=0;phase<cadence;++phase)for(bool pad:{false,true}) {
        ConformanceSession h;h.trace="sample-phase";unsigned selected{};
        for(unsigned now=phase;now<700;now+=cadence) {
            const auto out=h.poll(now,conformance_direction(static_cast<std::int16_t>(now>=100?4:0),pad,
                static_cast<std::uint16_t>(now>=300&&now<400?0x4000:0)));
            selected+=out.requested_id==2;
        }
        check(selected==1,"every30/60Hz sample phase issues exactly one native request for a held direction and physical face press");
    }
}
void production_order_and_boundary_matrix() {
    for(auto cadence:{17u,34u})for(auto code:{4,6,8,2})for(bool simultaneous:{false,true}) {
        ConformanceSession h;h.trace="triple-tail-order";h.poll(0);
        h.poll(cadence,conformance_direction(static_cast<std::int16_t>(code),false));h.poll(2*cadence);
        if(!simultaneous) {
            h.poll(3*cadence,{0x0100});const auto early=h.poll(4*cadence,{0x4000});
            check(!early.authored_playback&&h.session.diagnostic().recognition_count==0,
                "an intervening press blocks the base released-pair row before the either-order triple completes");
        }
        const auto out=h.poll(5*cadence,conformance_direction(static_cast<std::int16_t>(code),false,0x4000));
        const auto& d=h.session.diagnostic();
        check(d.recognition_count==1&&d.recognition[0].request.form==InputForm::repeated_direction&&
            d.recognition[0].request.recognition->footprint.count==4&&out.unsupported&&!out.authored_playback,
            "actual Session ordered triple allows either terminal order or simultaneous tail, ignoring unrelated presses without inventing a native actor");
    }
    for(auto age:{332u,333u,334u})for(bool pad:{false,true}) {
        ConformanceSession h;h.trace="ground-triple-boundary";h.admission.pose=SkatePose::riding;
        h.poll(0);h.poll(10,conformance_direction(2,pad));h.poll(20);h.poll(30,conformance_direction(2,pad));
        const auto out=h.poll(age+10,conformance_direction(2,pad,0x0200));
        check((out.ground_turn.serial!=0)==(age<333),
            "published ground-turn triple uses its own strict333ms query, without borrowing the Manual55ms allowance");
    }
    for(auto cadence:{17u,34u}) {
        ConformanceSession h;h.trace="revert-busy-expiry";h.admission.pose=SkatePose::riding;
        h.poll(0);h.poll(cadence,{0x1000});
        const auto request=h.poll(2*cadence,{0x1000,0,0,0,255});
        const auto& before=h.session.diagnostic();
        check(!request.action.owned&&before.recognition_count==1&&
            before.recognition[0].request.action==InputAction::revert&&before.recognition[0].request.deadline==2*cadence+1,
            "live RT route retains its current fresh-only handoff rather than claiming an installed source revert table");
        const auto next=h.poll(3*cadence,{0x1000,0,0,0,255});
        const auto& after=h.session.diagnostic();
        check(!next.action.owned&&after.dispatched==1&&after.dispatch[0].disposition==InputDisposition::expired,
            "a busy native owner cannot silently turn the one-millisecond RT handoff into reference20/200ms query parity");
    }
}

void production_queue_and_release_matrix() {
    for(auto cadence:{17u,34u})for(bool land:{false,true}) {
        ConformanceSession h;h.trace="pending-readiness";h.admission.flip_ready=false;
        h.poll(0);h.poll(cadence,{0x4000,-25000});const auto original=h.session.diagnostic().recognition[0].request;
        h.poll(2*cadence,{0,-25000});h.poll(1000,{0,-25000});
        if(land)h.admission.pose=SkatePose::riding;
        h.admission.flip_ready=true;const auto out=h.poll(1000+cadence,{0,-25000});
        const auto& d=h.session.diagnostic();
        check(land?!out.requested_id:out.requested_id==2,"original queued flip has no invented readiness expiry, while landing cancels before actuation");
        check(land ? d.dispatched==0 : d.dispatched==1 && d.dispatch[0].request.recognized_at==original.recognized_at &&
            d.dispatch[0].request.first_input_at==original.first_input_at && d.dispatch[0].disposition==InputDisposition::started,
            "readiness preserves original identity; flight withdrawal clears pending work before dispatch without fabricating a callback receipt");
    }
    for(auto cadence:{17u,34u}) {
        ConformanceSession h;h.trace="held-grab-release";h.poll(0);
        const auto start=h.poll(cadence,{0x2000});check(start.action.owned&&start.action.trick==ArcadeTrick::basic_bs_grab,
            "default classic Session publishes the existing held basic primitive, not a named grab identity");
        check(h.poll(cadence+1000,{0x2000}).action.pad.right_trigger==255,"held primitive remains active without a second physical press");
        const auto release=h.poll(cadence+1100);
        check(release.action.owned&&release.action.pad.right_trigger==0,"physical release enters the existing neutral tail without claiming a reference minimum-init gate");
        check(!h.poll(cadence+1134).action.owned,"existing neutral tail releases channels at its fixed34ms boundary");
        check(!h.poll(cadence+1200).action.requested,"a released face cannot replay the primitive");
    }
    // A simultaneous match is separate from native channel readiness.
    ConformanceSession h;h.trace="simultaneous-queue";h.poll(0);h.poll(10,{0,-25000});
    const auto out=h.poll(20,{0x6000,-25000});const auto& d=h.session.diagnostic();
    check(d.recognition_count==2&&d.recognition[0].request.action==InputAction::flip&&d.recognition[1].request.action==InputAction::grab&&
        out.requested_id==2&&h.session.pending_grab().recognized_at==20,
        "the live Session recognizes independent simultaneous rows in table order while only the first actor starts");
    for(unsigned now=30;now<1000;now+=17)h.poll(now,{0,-25000});
    const auto later=h.poll(1000,{0,-25000});
    check(h.session.pending_grab().recognized_at==20&&!later.action.owned&&later.action.result==DispatchResult::unavailable,
        "released waiting primitive retains semantic intent but cannot invent its missing initialization/out actor");
}

void production_session_boundary_matrix() {
    for(auto cadence:{17u,34u})for(unsigned actor=0;actor<5;++actor)for(bool rollback:{false,true}) {
        ConformanceSession h;h.trace=rollback?"session-clock-rollback":"session-bail";
        h.admission.export_rail_requests=true;
        h.admission.pose=actor==1||actor==2||actor==3?SkatePose::air:SkatePose::riding;
        const auto pose=h.admission.pose;
        const PhysicalInputSample commands[]{{0x1000},{0x4000,-25000},{0x2000},{0x8000},{0,0,0,0,255}};
        const auto physical=commands[actor];h.poll(1000);const auto start=h.poll(1000+cadence,physical);
        check(actor==0||actor==1?start.authored_playback:actor==3?start.rail.held:start.action.owned,
            "current Session endpoint starts before its independent bail or clock-loss boundary");
        const auto recognized=h.session.diagnostic().recognized;
        if(!rollback)h.admission.pose=SkatePose::bail;
        const auto stopped=h.poll(rollback?999:1000+2*cadence,physical);
        check(!stopped.authored_playback&&!stopped.action.owned&&!stopped.rail.held&&!stopped.command_busy,
            "bail and clock rollback withdraw supported actors before another native dispatch");
        check(h.session.diagnostic().recognition_count==0 && h.session.diagnostic().recognition_omitted==0 &&
            h.session.diagnostic().recognized==recognized,
            "a hard Session boundary cannot republish the preceding poll's recognition attempts");
        h.admission.pose=pose;const auto held=h.poll(1000+3*cadence,physical);
        check(!held.authored_playback&&!held.action.owned&&!held.rail.held,
            "hard-boundary held input is rebaselined without becoming a new command");
    }
}

struct ConformanceFacade {
    NativeInputState producer; NativeInputOwner owner{1,2,3,4,5}; ArcadeInput input{true};
    std::vector<PreparedGesture> prepared{catalog()}; unsigned cadence{17};const char* trace{"bound-facade"};
    ConformanceFacade(){input.enable(true);input.set_recognition_capture(0,true);}
    NativeGesture::Output poll(std::uint64_t now,unsigned state,PhysicalInputSample physical={},bool publish=true) {
        if(publish) {
            NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=1;
            sample.words[1]=state==100?0xc000u:0u;
            const auto frame=producer.observe(now,owner,sample,{true,-1});
            input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
        }
        const auto pose=state==200?SkatePose::air:state==401?SkatePose::grind:SkatePose::riding;
        const auto out=input.poll(0,now,physical.buttons,physical.x,physical.y,0,0,true,true,false,0,
            physical.lt>30||physical.rt>30,false,true,now/cadence,false,prepared,false,false,pose,
            false,false,false,false,true,false,physical.lt,physical.rt,ActionExperiment::classic,true,true,false);
        if(conformance_record)conformance_record(trace,now,out,*input.input_diagnostic(0),*input.history(0));
        return out;
    }
};
void production_facade_matrix() {
    for(auto cadence:{17u,34u})for(unsigned actor=0;actor<5;++actor)for(unsigned boundary=0;boundary<5;++boundary) {
        ConformanceFacade h;h.cadence=cadence;h.trace="bound-owner-lifecycle";
        const auto state=actor==1||actor==2||actor==3?200u:100u;
        const PhysicalInputSample commands[]{{0x1000},{0x4000,-25000},{0x2000},{0x8000},{0,0,0,0,255}};
        const auto physical=commands[actor];h.poll(1000,state);const auto start=h.poll(1000+cadence,state,physical);
        check(actor==0||actor==1?start.authored_playback:actor==3?start.rail.held:start.action.owned,
            "actual bound facade publishes each supported jump/flip/basic-grab/rail/revert endpoint");
        auto now=1000+2*cadence;
        if(boundary==0)h.input.enable(false);
        if(boundary==1){auto menu=physical;menu.buttons|=0x10;h.poll(now,state,menu);}
        if(boundary==2)now=1000+cadence+151;
        if(boundary==3)++h.owner.core;
        const auto stopped=h.poll(now,boundary==4?999u:state,physical,boundary!=2);
        check(!stopped.authored_playback&&!stopped.action.owned&&!stopped.rail.held&&!stopped.command_busy,
            "OFF/menu/provider loss/native owner change/unknown state withdraws all currently published command owners");
        check(h.input.input_diagnostic(0)->recognition_count==0 && h.input.input_diagnostic(0)->recognition_omitted==0,
            "facade cancellation leaves no old current-poll recognition rows for a telemetry consumer");
        if(boundary==0)h.input.enable(true);
        now+=cadence;const auto held=h.poll(now,state,physical);
        check(!held.authored_playback&&!held.action.owned&&!held.rail.held,
            "restored actual facade baselines a held-through command rather than manufacturing a fresh press");
        h.poll(now+cadence,state);const auto rearmed=h.poll(now+2*cadence,state,physical);
        check(actor==0||actor==1?rearmed.authored_playback:actor==3?rearmed.rail.held:rearmed.action.owned,
            "a genuine release and fresh press rearm the supported endpoint through the actual native-bound caller");
    }
    for(auto cadence:{17u,34u}) {
        ConformanceFacade jump;jump.cadence=cadence;jump.trace="bound-jump-release";
        jump.poll(1000,100);jump.poll(1000+cadence,100,{0x1000});
        const auto released=jump.poll(1000+2*cadence,200);
        check(released.jump.released&&released.jump.setup_admitted&&released.jump.pressed_at==1000+cadence&&
            released.jump.released_at==1000+2*cadence&&!released.jump.from_rail,
            "actual bound ordinary jump retains its admitted original press and physical release across takeoff");
        check(!jump.poll(1000+3*cadence,200).jump.released,"bound release is published once independently of the held setup actor");
        ConformanceFacade rail;rail.cadence=cadence;rail.trace="bound-rail-handoff";
        rail.poll(1000,200);const auto ticket=rail.poll(1000+cadence,200,{0x8000});
        const auto pad=rail.input.sample_native_rail(ticket.rail,1000+cadence,true);
        check(ticket.rail.held&&pad.owns_right_stick&&!pad.button_mask,
            "actual generic rail ticket reaches the existing alignment adapter without claiming named grind identity");
        const auto release=rail.poll(1000+2*cadence,200);
        check(!release.rail.held&&release.rail.request.input_sequence==ticket.rail.request.input_sequence&&
            release.rail.request.physical_released_at==1000+2*cadence,
            "actual rail release preserves ticket identity and its independently observed physical edge");
        ConformanceFacade contact;contact.cadence=cadence;contact.trace="bound-rail-exit-veto";
        contact.poll(1000,401);const auto exit=contact.poll(1000+cadence,401,{0x1000});
        check(!exit.jump.setup_started&&!exit.authored_playback&&!exit.requested_id&&
            contact.input.input_diagnostic(0)->recognition_count==0,
            "bound rail-family safety veto does not publish an ordinary crouch/exit actor from caller may-start alone");
        ConformanceFacade ground;ground.cadence=cadence;ground.trace="bound-ground-turn";
        ground.poll(1000,100);ground.poll(1000+cadence,100,{0,0,-25000});
        ground.poll(1000+2*cadence,100);ground.poll(1000+3*cadence,100,{0,0,-25000});
        const auto turn=ground.poll(1000+4*cadence,100,{0x0200,0,-25000});
        check(turn.ground_turn.serial&&turn.ground_turn.requested_at==1000+4*cadence&&!turn.authored_playback&&!turn.action.owned,
            "active down/down/shoulder table publishes one typed ground-turn command through the bound facade");
    }
}

void auxiliary_mode_contract() {
    for(auto cadence:{17u,34u})for(auto mode:{ActionExperiment::off,ActionExperiment::observe,ActionExperiment::grabs,
        ActionExperiment::revert_frontside,ActionExperiment::revert_backside,ActionExperiment::classic}) {
        ConformanceSession h;h.admission.pose=SkatePose::riding;h.admission.action_mode=mode;
        h.poll(1000);const auto out=h.poll(1000+cadence,{0,0,0,0,255});
        const bool revert=mode==ActionExperiment::classic||mode==ActionExperiment::revert_frontside||mode==ActionExperiment::revert_backside;
        check(out.action.owned==revert&&out.action.requested==revert&&out.action.consume_rt==revert,
            "RT ownership, recognition and physical masking use the same published action mode");
        check(h.session.diagnostic().recognition_count==(revert?1u:0u),
            "grab-only and observation modes do not publish a ground RT action table");
        for(const auto& event:h.session.history().events())if(event.token==InputToken{InputSource::triggers,1,InputEdge::press})
            check(event.owners==(revert?InputConsumption::regular:0u),"an unpublished RT table leaves its physical event category unconsumed");
        if(revert) {
            check(out.action.request_kind==(mode==ActionExperiment::revert_backside?4u:3u),
                "existing frontside/backside recipe selection remains separate from observed native identity");
            check(!h.poll(1000+cadence+235,{0,0,0,0,255}).action.owned,
                "held RT cannot renew the existing absolute235ms recipe lifetime");
        }
    }
    // The actual caller supports selecting this experiment only while disabled.
    for(auto cadence:{17u,34u})for(auto mode:{ActionExperiment::grabs,ActionExperiment::classic,ActionExperiment::revert_backside}) {
        NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};ArcadeInput input{true};
        input.enable(true);input.set_recognition_capture(0,true);
        const auto poll=[&](std::uint64_t now,std::uint8_t rt) {
            NativeRequestSample sample{};sample.valid=true;sample.state=100;sample.crouch_mode=1;sample.words[1]=0xc000;
            const auto frame=producer.observe(now,owner,sample,{true,-1});
            input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
            return input.poll(0,now,0,0,0,0,0,true,true,false,0,rt>30,false,false,now/cadence,false,{},false,false,
                SkatePose::riding,false,false,false,false,true,false,0,rt,mode,true,true,false);
        };
        poll(1000,0);const auto out=poll(1000+cadence,255);
        const bool enabled=mode!=ActionExperiment::grabs;
        check(out.action.owned==enabled&&out.action.consume_rt==enabled&&
            input.input_diagnostic(0)->recognition_count==(enabled?1u:0u),
            "native-bound facade cannot start a hidden revert actor in the grab-only experiment");
    }
}

void auxiliary_release_handoff_contract() {
    for(bool cancel:{false,true}) {
        ConformanceSession h;h.poll(0);check(h.poll(10,{0x2000}).action.owned,"known held primitive starts before a physical release handoff");
        check(h.poll(20).action.pad.right_trigger==0,"release enters the existing software neutral tail");
        const auto blocked=h.poll(30,{0x2000});
        const auto original=h.session.pending_grab();
        check(original.action==InputAction::grab&&original.physical_pressed_at==30&&original.recognized_at==30&&
            blocked.action.owned&&blocked.action.pad.right_trigger==0,
            "a real repress during neutral retains its own edge while the old actor still owns release");
        if(cancel)h.admission.external_owner=true;
        const auto next=h.poll(54,{0x2000});
        if(cancel) {
            check(!next.action.owned&&h.session.pending_grab().action==InputAction::none,
                "known competing ownership cancels both release and queued repress without a stale handoff");
            h.admission.external_owner=false;
            check(!h.poll(60,{0x2000}).action.owned,"held input through cancellation cannot revive the withdrawn repress");
            h.poll(70);check(h.poll(80,{0x2000}).action.owned,"fresh release/press rearms after ownership cancellation");
        } else {
            const auto& d=h.session.diagnostic();
            check(next.action.owned&&next.action.pad.right_trigger==255&&d.dispatched==1&&
                d.dispatch[0].request.recognized_at==original.recognized_at&&
                d.dispatch[0].request.physical_pressed_at==original.physical_pressed_at,
                "known software release completion admits the original queued repress once, without retimestamping");
            h.poll(100);check(!h.poll(134).action.owned,"second release independently finishes its own software neutral tail");
        }
    }
    ConformanceSession h;h.admission.pose=SkatePose::riding;h.poll(0);
    check(h.poll(10,{0,0,0,0,255}).action.owned,"RT begins one existing native recipe request");
    check(h.poll(20).action.owned,"RT is a press request; physical release alone does not cancel an admitted recipe");
    h.poll(30,{0,0,0,0,255});const auto& waiting=h.session.diagnostic();
    check(waiting.recognition_count==1&&waiting.recognition[0].request.recognized_at==30,
        "a second genuine RT edge is distinct from a held repeat while the first actor is busy");
    h.poll(40,{0,0,0,0,255});
    check(h.session.diagnostic().dispatched==1&&h.session.diagnostic().dispatch[0].disposition==InputDisposition::expired,
        "existing bounded RT handoff expiry cannot renew the already active recipe");
    check(!h.poll(245,{0,0,0,0,255}).action.owned,"repeated RT cannot extend the first absolute235ms ownership window");
    check(!h.poll(300,{0,0,0,0,255}).action.requested,"held RT does not recreate an expired pending request");
    h.poll(310);check(h.poll(320,{0,0,0,0,255}).action.owned,"release and fresh press after completion starts one new recipe");
}

}
int main() {
    auxiliary_mode_contract(); auxiliary_release_handoff_contract();
    production_session_boundary_matrix(); production_facade_matrix(); production_air_query_matrix(); production_order_and_boundary_matrix(); production_queue_and_release_matrix();
    manual_exit_jump_ownership(); queued_grab_identity(); airborne_setup_eligibility(); repeated_air_safety(); jump_release_metadata(); native_adapters(); one_journal_and_boundaries(); unsupported_branches(); unsupported_contexts(); balance_transition_contracts(); manual_readiness_contracts();
    std::cout << checks << " production input session checks passed\n";
    return 0;
}
