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
    check(h.session.history().events().size() >= 4, "manual uses the session's single journal");
    h.admission.pose = SkatePose::air; h.poll(51, {0x4000, -25000, 0});
    h.poll(68, {0x2000}); h.poll(85, {0x8000}); h.poll(102, {0, 0, 0, 0, 255});
    bool button{}, vertical{}, trigger{};
    for (const auto& event : h.session.history().events()) {
        button = button || event.token.source == InputSource::buttons;
        vertical = vertical || event.token.source == InputSource::vertical_stick;
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

}
int main() {
    airborne_setup_eligibility(); repeated_air_safety(); jump_release_metadata(); native_adapters(); one_journal_and_boundaries(); unsupported_branches(); unsupported_contexts(); balance_transition_contracts(); manual_readiness_contracts();
    std::cout << checks << " production input session checks passed\n";
}
