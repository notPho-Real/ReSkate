#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
void released_direction() {
    for(bool strict:{false,true})for(bool pad:{false,true})for(bool barrier:{false,true}) {
        InputSession session;InputAdmission a{};a.gameplay=a.authored=a.action_fresh=a.inspected_stance=true;
        a.pose=SkatePose::air;a.action_mode=ActionExperiment::classic;a.input_policy=strict?InputPolicy::strict():InputPolicy::production();
        const std::array catalog{PreparedGesture{2,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};a.catalog=catalog;
        session.poll(0,{},0,0,a);session.poll(10,{static_cast<std::uint16_t>(pad?4:0),static_cast<std::int16_t>(pad?0:-25000),0},0,0,a);
        session.poll(20,{},0,0,a);
        if(barrier){session.poll(23,{0x8000},0,0,a);session.poll(25,{},0,0,a);}
        const auto out=session.poll(30,{0x4000},0,0,a);
        check((out.requested_id==2)==!barrier,"released pad/stick direction reaches the active matcher; a real intervening face press rejects it");
        for(const auto& event:session.history().events())check(!(event.token.source==InputSource::buttons && event.token.code<4),
            "raw pad bits cannot appear as unrelated duplicate command presses");
    }
    for(bool strict:{false,true})for(bool handoff:{false,true}) {
        InputSession session;InputAdmission a{};a.gameplay=a.authored=true;a.pose=SkatePose::air;
        a.input_policy=strict?InputPolicy::strict():InputPolicy::production();
        const std::array catalog{PreparedGesture{2,false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};a.catalog=catalog;
        session.poll(0,{},0,0,a);session.poll(10,{4},0,0,a);session.poll(12,{4,0,25000},0,0,a);session.poll(14,{4,0,-25000},0,0,a);
        session.poll(20,{0,static_cast<std::int16_t>(handoff?-25000:0),0},0,0,a);
        const auto out=session.poll(30,{0x4000,static_cast<std::int16_t>(handoff?-25000:0),0},0,0,a);
        check(out.requested_id==2,"stick motion behind a held pad cannot become an unrelated command, and same-direction device handoff creates no fake press");
        unsigned presses{};for(const auto& event:session.history().events())if(event.token.source==InputSource::direction && event.token.edge==InputEdge::press)++presses;
        check(presses==1,"general recognition records one authoritative logical direction rather than competing device views");
    }
}
void typed_balance() {
    for(auto kind:{InputBalanceKind::manual,InputBalanceKind::nose,InputBalanceKind::grind,InputBalanceKind::slide,InputBalanceKind::lip})
        for(bool strict:{false,true}) {
            const bool vertical=kind==InputBalanceKind::manual || kind==InputBalanceKind::nose;
            const std::int16_t first=vertical?8:4, second=vertical?2:6;
            const InputBalanceControls controls{kind,{InputSource::direction,first,InputEdge::press},{InputSource::direction,second,InputEdge::press},true};
            const auto policy=strict?InputPolicy::strict():InputPolicy::production();
            InputEvents events;events.sample(0,{},controls,false,policy);
            events.sample(10,{static_cast<std::uint16_t>(vertical?1:4)},controls,false,policy);
            check(events.history().events().empty() && events.held({strict?InputSource::direction:InputSource::pad,first,InputEdge::press}),
                "typed correction exclusion preserves physical held state without command events");
            events.sample(20,{},controls,false,policy);events.sample(30,{static_cast<std::uint16_t>(vertical?2:8)},controls,false,policy);
            check(events.history().events().empty(),"both correction directions and their releases are excluded");
            events.sample(40,{static_cast<std::uint16_t>((vertical?2:8)|0x2000),0,0,0,255},controls,false,policy);
            check(events.history().events().size()==2,"unrelated face and trigger events survive every typed balance exclusion");
            InputSession session;InputAdmission a{};a.gameplay=true;a.pose=vertical?SkatePose::manual:SkatePose::grind;
            a.observed_lip=kind==InputBalanceKind::lip;a.balance_controls=controls;a.input_policy=policy;
            session.poll(0,{},0,0,a);session.poll(10,{static_cast<std::uint16_t>(vertical?1:4)},0,0,a);
            check(session.history().events().empty(),"the active session forwards admitted typed controls rather than a Manual-only boolean");
            a.balance_controls={};session.poll(20,{static_cast<std::uint16_t>(vertical?2:8)},0,0,a);
            check(session.history().events().empty(),"recording uses the preceding published exclusion set");
            session.poll(30,{static_cast<std::uint16_t>(vertical?2:8)},0,0,a);
            check(session.history().events().empty(),"held-through-exclusion controls cannot become new presses");
            session.poll(40,{},0,0,a);session.poll(50,{static_cast<std::uint16_t>(vertical?1:4)},0,0,a);
            check(!session.history().events().empty(),"a new edge records normally after the exclusion ends");
        }
    InputEvents buttons;const InputBalanceControls face{InputBalanceKind::grind,
        {InputSource::buttons,12,InputEdge::press},{InputSource::buttons,14,InputEdge::press},true};
    buttons.sample(0,{},face,false,InputPolicy::production());buttons.sample(10,{0x5000},face,false,InputPolicy::production());
    check(buttons.history().events().empty() && buttons.held(face.first) && buttons.held(face.second),
        "published balance controls are actual logical tokens rather than fixed cardinal direction assumptions");
    buttons.sample(20,{0x7000},face,false,InputPolicy::production());
    check(buttons.history().events().size()==1 && buttons.history().events()[0].token.code==13,
        "a custom control pair cannot suppress an unrelated face button");
    InputEvents inactive;auto unadmitted=face;unadmitted.admitted=false;
    inactive.sample(0,{},unadmitted,false,InputPolicy::production());inactive.sample(10,{0x5000},unadmitted,false,InputPolicy::production());
    check(inactive.history().events().size()==2,"a named balance kind alone cannot manufacture admitted correction exclusions");
}
NativeGesture::Output board_poll(ArcadeInput& input,std::uint64_t now,std::uint16_t buttons,bool allowed=true,bool offboard=false) {
    return input.poll(0,now,buttons,0,0,0,0,true,false,false,0,false,allowed,false,0,false,{},false,false,
        SkatePose::riding,false,false,offboard);
}
void active_board() {
    for(std::uint16_t first:{std::uint16_t{0x100},std::uint16_t{0x200}})for(unsigned gap:{119u,120u,121u,200u,399u,400u,401u}) {
        ArcadeInput input;input.enable(true);board_poll(input,0,0);board_poll(input,10,first);
        const auto out=board_poll(input,10+gap,0x300);
        check((out.phase==3)==(gap<400),"the public board caller uses strict400ms historical overlap, independently of the120ms native pulse");
        if(gap<400) {
            check(out.buttons==0x8000,"the existing native board adapter receives one owned pulse");
            check(board_poll(input,10+gap+119,0x300).phase==3 && board_poll(input,10+gap+120,0x300).phase==0 &&
                board_poll(input,10+gap+121,0x300).buttons==0,
                "native actuation keeps its original bounded120ms pulse");
        }
    }
    for(bool overlap:{false,true})for(bool offboard:{false,true}) {
        ArcadeInput input;input.enable(true);board_poll(input,0,0,false,offboard);board_poll(input,10,0x100,false,offboard);
        board_poll(input,20,overlap?0x300:0,false,offboard);board_poll(input,25,overlap?0:0x200,false,offboard);
        board_poll(input,30,0,false,offboard);const auto out=board_poll(input,40,0,true,offboard);
        check((out.phase==3)==overlap,"historical overlap after release reaches the active on/off-board caller; nonoverlap fails");
    }
    ArcadeInput extra;extra.enable(true);board_poll(extra,0,0);board_poll(extra,10,0x100);board_poll(extra,15,0x8100);
    board_poll(extra,20,0x100);check(board_poll(extra,30,0x300).phase==3,
        "historical overlap ignores unrelated historical presses, distinct from held-pair adjacency");
    for(auto pattern:{InputBoardPattern::single_control,InputBoardPattern::unavailable,InputBoardPattern::shoulder_overlap}) {
        InputSession session;InputAdmission a{};a.gameplay=a.may_toggle_board=true;a.pose=SkatePose::riding;
        a.board_binding={pattern,0x0080};session.poll(0,{},0,0,a);const auto out=session.poll(10,{0x0080},0,0,a);
        check((out.phase==3)==(pattern==InputBoardPattern::single_control),"alternate logical layout requires an explicit binding; no console mapping is guessed");
    }
}
void retained_grab() {
    struct Sink {unsigned calls{};bool ready{};InputRequest last{};
        static InputDisposition receive(void* p,const InputRequest& r) noexcept {auto& self=*static_cast<Sink*>(p);++self.calls;self.last=r;return self.ready?InputDisposition::started:InputDisposition::waiting;}
    } sink;
    UnifiedInput input;InputEnvironment e{InputContext::air,true};const InputAdapter adapter{&sink,Sink::receive};
    input.poll(0,{},e,adapter);input.poll(10,{0x2000},e,adapter);input.poll(20,{},e,adapter);
    input.poll(1000,{},e,adapter);check(sink.last.recognized_at==10 && sink.last.unbounded,"release and generic elapsed200ms cannot delete or retimestamp an ordinary action");
    sink.ready=true;input.poll(1010,{},e,adapter);check(sink.last.recognized_at==10,"delayed adapter initialization receives the original queued identity after release");
    InputSession session;InputAdmission a{};a.gameplay=a.authored=a.action_fresh=a.inspected_stance=a.may_start=true;
    a.pose=SkatePose::riding;a.action_mode=ActionExperiment::classic;
    const std::array catalog{PreparedGesture{1,false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}}};a.catalog=catalog;
    session.poll(0,{},0,0,a);session.poll(10,{0x1000},0,0,a);a.pose=SkatePose::air;
    session.poll(20,{0x2000},0,0,a);session.poll(30,{},0,0,a);a.tick=30;session.poll(40,{},0,0,a);
    const auto waiting=session.poll(50,{},0,0,a);
    check(waiting.action.requested && !waiting.action.owned && waiting.action.result==DispatchResult::unavailable &&
        session.pending_grab().recognized_at==20,
        "missing released-grab initialization contract remains explicit without replaying the fixed500ms recipe");
    const auto later=session.poll(1000,{},0,0,a);
    check(session.pending_grab().recognized_at==20 && !later.action.owned,"missing init capability cannot retimestamp intent or create an indefinite post-release native owner");
    a.pose=SkatePose::riding;const auto landed=session.poll(1010,{},0,0,a);
    check(!landed.action.requested && !landed.action.owned && session.pending_grab().action==InputAction::none,"landing cancels the retained air action before unsupported initialization can own input");
}
void delayed_flip_readiness() {
    for (unsigned cadence : {17u,34u}) for (bool landed : {false,true}) {
        InputSession session; InputAdmission admission{};
        admission.gameplay=admission.authored=admission.action_fresh=admission.inspected_stance=true;
        admission.pose=SkatePose::air; admission.action_mode=ActionExperiment::classic;
        admission.flip_ready=false;
        const std::array catalog{PreparedGesture{2,false,GestureContext::ground,
            std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}}}};
        admission.catalog=catalog;
        unsigned starts{};
        for (unsigned now=0;now<900;now+=cadence) {
            admission.tick=now/cadence;
            admission.flip_ready=now>=500;
            if (landed && now>=250) admission.pose=SkatePose::riding;
            const auto out=session.poll(now,{static_cast<std::uint16_t>(now>=100 && now<170?0x4000:0),
                static_cast<std::int16_t>(now>=100 && now<200?-25000:0)},0,0,admission);
            if (out.requested_id) {
                ++starts;
                check(out.requested_id==2 && now>=500 && now<500+cadence,
                    "a recognized flip survives face release and the obsolete200ms handoff timeout, and starts only when native readiness arrives");
            }
        }
        check(starts==(landed?0u:1u),"landing cancels a retained flip; readiness cannot replay it or create a second request");
    }
}

void air_setup_conflict() {
    for (bool released : {false,true}) {
        InputSession session;InputAdmission admission{};
        admission.gameplay=admission.authored=admission.action_fresh=admission.inspected_stance=admission.may_start=true;
        admission.pose=SkatePose::air;admission.action_mode=ActionExperiment::classic;
        session.poll(0,{},0,0,admission);
        const auto busy=session.poll(10,{0x3000},0,0,admission);
        check(!busy.authored_playback && !busy.action.owned && busy.action.result==DispatchResult::busy,
            "an airborne setup button cannot start a ground recipe; the native grab ownership conflict remains a waiting gate");
        check(session.pending_grab().recognized_at==10,"a reversible physical button conflict retains the original grab intent");
        const auto ready=session.poll(20,{static_cast<std::uint16_t>(released?0:0x2000)},0,0,admission);
        check(!ready.jump.released && !ready.jump.setup_admitted,
            "releasing a setup button pressed in air cannot fabricate a native jump release");
        check(released?(!ready.action.owned && ready.action.result==DispatchResult::unavailable && session.pending_grab().recognized_at==10):ready.action.owned,
            "a held grab starts after the reversible conflict; an already released grab waits explicitly for native initialization support");
    }
}

void timing_policies() {
    for(bool strict:{false,true})for(bool air:{false,true})for(unsigned age:{219u,220u,221u,274u,275u,276u,399u,400u,401u}) {
        InputSession session;InputAdmission a{};a.gameplay=a.manual_safe=true;a.pose=air?SkatePose::air:SkatePose::riding;
        a.input_policy=strict?InputPolicy::strict():InputPolicy::production();
        session.poll(0,{},0,0,a);session.poll(10,{0,0,25000},0,0,a);const auto out=session.poll(10+age,{0,0,-25000},0,0,a);
        const bool expected=air?age<400:strict?age<220:age<=220+55;
        check(out.manual.owns==expected,"canonical query ages stay strict; the separately named55ms allowance applies only to production ground sampling");
    }
}

void hard_boundaries() {
    for(std::uint16_t menu:{std::uint16_t{0x10},std::uint16_t{0x20}}) {
        InputSession session;InputAdmission a{};a.gameplay=a.action_fresh=a.inspected_stance=true;a.pose=SkatePose::air;a.action_mode=ActionExperiment::classic;
        session.poll(0,{},0,0,a);check(session.poll(10,{0x2000},0,0,a).action.owned,"hold owns input before menu boundary");
        const auto out=session.poll(20,{static_cast<std::uint16_t>(menu|0x2000)},123,456,a);
        check(!out.action.owned && !out.phase && out.right_x==123 && out.right_y==456,"menu edges cancel every synthetic owner and restore physical channels immediately");
        check(!session.poll(30,{0x2000},0,0,a).action.owned,"held controls cannot rearm across a menu boundary");
    }
    InputSession pushing;InputAdmission push{};push.gameplay=push.may_push=true;push.pose=SkatePose::air;
    pushing.poll(0,{},0,0,push);check(!(pushing.poll(10,{0,25000,0},0,0,push).buttons & 0x4000u),
        "a lingering ground push deadline cannot synthesize a face button while airborne");
    push.pose=SkatePose::riding;check(pushing.poll(20,{0,25000,0},0,0,push).buttons & 0x4000u,
        "the proven automatic native push remains available in riding context");
    InputSession unknown;InputAdmission a{};a.gameplay=a.authored=a.may_start=true;
    const std::array catalog{PreparedGesture{1,false,GestureContext::ground,std::vector<AuthoredStickStep>{{0,0,-32767},{10,0,0}}}};a.catalog=catalog;
    unknown.poll(0,{},0,0,a);check(!unknown.poll(10,{0x1000},0,0,a).authored_playback,"unknown startup cannot borrow a lingering ground-ready flag");
}

}
int main(){hard_boundaries();air_setup_conflict();delayed_flip_readiness();timing_policies();released_direction();typed_balance();active_board();retained_grab();std::cout<<checks<<" active input review checks passed\n";}
