#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* message) { ++checks; if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); } }
struct Sink {
    unsigned calls{}; InputRequest last{};
    static InputDisposition receive(void* state, const InputRequest& request) noexcept {
        auto& self = *static_cast<Sink*>(state); ++self.calls; self.last = request; return InputDisposition::unsupported;
    }
    InputAdapter adapter() noexcept { return {this, receive}; }
};
void normalization() {
    for (const int value : {13056, 13057, -12799, -12800}) {
        InputEvents events; events.sample(0, {}, false, false, InputPolicy::strict());
        events.sample(10, {0, static_cast<std::int16_t>(value), 0}, false, false, InputPolicy::strict());
        const bool active = value >= 13057 || value <= -12800;
        check(events.history().events().size() == (active ? 1u : 0u), "asymmetric axis quantization entry is exact");
        if (active) check(events.history().events()[0].token.source == InputSource::direction, "one canonical physical direction spends one ring entry");
    }
    for (unsigned mask = 1; mask < 16; ++mask) {
        InputEvents events; events.sample(0, {}, false, false, InputPolicy::strict());
        events.sample(10, {static_cast<std::uint16_t>(mask), -25000, 25000}, false, false, InputPolicy::strict());
        const int x = mask & 8u ? 1 : mask & 4u ? -1 : 0;
        const int y = mask & 2u ? -1 : mask & 1u ? 1 : 0;
        check(events.held_direction() == InputEvents::encode(x,y) && events.history().events().size() == 1,
            "pad override and all opposing-direction combinations use one canonical event");
        events.sample(20, {static_cast<std::uint16_t>(mask), 25000, -25000}, false, false, InputPolicy::strict());
        check(events.history().events().size() == 1, "pad-held stick motion creates no extra physical direction events");
    }
    InputEvents balance; balance.sample(0, {}, false, false, InputPolicy::strict());
    balance.sample(10, {1}, true, false, InputPolicy::strict());
    check(balance.history().events().empty() && balance.held({InputSource::direction,8,InputEdge::press}),
        "excluded balance direction remains physically held without duplicate history");
}
void sampled_windows() {
    for (unsigned step : {17u,34u}) for (unsigned phase = 0; phase < step; ++phase)
        for (unsigned gap : {200u,219u,220u,250u,399u,400u}) for (bool air : {false,true}) {
            UnifiedInput input; Sink sink; InputEnvironment env{air ? InputContext::air : InputContext::ground,true}; env.policy = InputPolicy::strict();
            std::uint64_t first{},second{};
            for (unsigned now = phase; now < 1000; now += step) {
                const auto direction = now < 100 ? 0 : now < 100 + gap ? 25000 : -25000;
                if (!first && now >= 100) first = now;
                if (!second && now >= 100 + gap) second = now;
                input.poll(now, {0,0,static_cast<std::int16_t>(direction)}, env, sink.adapter());
            }
            check((sink.calls != 0) == (second - first < (air ? 400u : 220u)),
                "all sampling phases use actual strict event ages rather than widening the source window");
        }
    for (unsigned gap : {219u,220u,221u,274u,275u,276u}) {
        UnifiedInput input; Sink sink; InputEnvironment env{InputContext::ground,true};
        input.poll(0,{},env,sink.adapter()); input.poll(10,{0,0,25000},env,sink.adapter()); input.poll(10+gap,{0,0,-25000},env,sink.adapter());
        check((sink.calls != 0) == (gap <= 275), "compatibility allowance remains an explicit independent policy");
    }
}
void preparation() {
    for (bool air : {false,true}) {
        InputSession session; InputAdmission admission{}; admission.gameplay = true; admission.manual_prepare = true;
        admission.pose = air ? SkatePose::air : SkatePose::riding; admission.input_policy = InputPolicy::strict();
        session.poll(0,{},0,0,admission); session.poll(10,{0,0,25000},0,0,admission);
        auto out = session.poll(110,{0,0,-25000},0,0,admission);
        check(!out.manual.owns && session.pending_manual().recognized_at == 110,
            "preparation-only recognized pair retains intent without native ownership");
        session.poll(200,{},0,0,admission);
        check(session.pending_manual().recognized_at == 110, "readiness observations do not retimestamp the selection");
        admission.manual_safe = true; admission.manual_prepare = false;
        out = session.poll(210,{},0,0,admission);
        check(out.manual.owns && session.pending_manual().action == InputAction::none,
            "same preparation pair executes once on fresh native readiness");
    }
}
void early_contact_readiness() {
    InputSession session; InputAdmission a{}; a.gameplay = a.manual_prepare = true; a.pose=SkatePose::riding;
    a.input_policy=InputPolicy::strict();
    session.poll(0,{},0,0,a); session.poll(10,{0,0,25000},0,0,a); session.poll(100,{0,0,-25000},0,0,a);
    a.pose=SkatePose::manual;
    check(!session.poll(110,{},0,0,a).manual.owns && session.pending_manual().recognized_at==100,
        "early raw contact without independent balance admission cannot erase prepared intent");
    a.manual_prepare=false; a.manual_safe=true;
    check(session.poll(120,{},0,0,a).manual.owns && session.pending_manual().action==InputAction::none,
        "fresh readiness may actuate the same selection after raw contact while retaining native acquisition guards");
    a.manual_balance_admitted=true;
    check(session.poll(130,{},0,0,a).manual.owns,"independent balance admission retains the existing native owner");
}

void grab_hold() {
    for (unsigned duration : {5u,50u,500u,1500u}) {
        InputSession session; InputAdmission a{}; a.gameplay = a.action_fresh = a.inspected_stance = true;
        a.action_mode = ActionExperiment::classic; a.pose = SkatePose::air;
        session.poll(0,{},0,0,a); auto out = session.poll(10,{0x2000},0,0,a);
        check(out.action.owned && out.action.pad.right_trigger == 255, "physical grab press reaches the held native primitive");
        out = session.poll(10+duration,{0x2000},0,0,a);
        check(out.action.owned && out.action.pad.right_trigger == 255, "hold duration follows physical state rather than the old fixed recipe");
        out = session.poll(11+duration,{},0,0,a);
        check(out.action.owned && out.action.pad.right_trigger == 0, "release begins the adapter's explicit neutral phase");
        check(!session.poll(45+duration,{},0,0,a).action.owned, "neutral release completes without a replayed press");
    }
}
void rail_tables() {
    constexpr std::array first{15u,15u,15u,13u,13u,13u,14u,14u,14u};
    constexpr std::array second{15u,14u,13u,13u,14u,15u,14u,13u,15u};
    for (unsigned geometry = 0; geometry < 16; ++geometry) for (unsigned pair = 0; pair < first.size(); ++pair)
        for (unsigned gap : {299u,300u,301u}) {
            UnifiedInput input; Sink sink; InputEnvironment env{InputContext::rail,true}; env.policy = InputPolicy::strict();
            env.rail.phase = InputRailPhase::established; env.rail.fresh = env.rail.ready = env.rail.geometry_known = true;
            env.rail.right = (geometry & 1u) != 0; env.rail.parallel = (geometry & 2u) != 0;
            env.rail.backwards = (geometry & 4u) != 0; env.rail.regular = (geometry & 8u) != 0;
            input.poll(0,{},env,sink.adapter()); input.poll(10,{static_cast<std::uint16_t>(1u << first[pair])},env,sink.adapter());
            input.poll(20,{},env,sink.adapter()); input.poll(10+gap,{static_cast<std::uint16_t>(1u << second[pair])},env,sink.adapter());
            check((sink.calls != 0) == (gap < 300), "every established family and geometry uses the strict300ms query");
            if (sink.calls) check(sink.last.variant == pair+1 && sink.last.geometry_known && sink.last.geometry == geometry &&
                sink.last.action == InputAction::rail_change, "semantic family and actual geometry remain separate from native execution");
        }
    constexpr std::array<std::int16_t,8> directions{9,3,1,7,8,6,4,2};
    for (unsigned row = 0; row < directions.size(); ++row) for (unsigned gap : {999u,1000u,1001u}) {
        UnifiedInput input; Sink sink; InputEnvironment env{InputContext::rail,true}; env.policy = InputPolicy::strict();
        env.rail.phase = InputRailPhase::early; env.rail.fresh = env.rail.ready = true;
        const auto code = directions[row]; const auto x = (code == 9 || code == 3 || code == 6) ? 25000 : (code == 7 || code == 1 || code == 4) ? -25000 : 0;
        const auto y = code >= 7 ? 25000 : code <= 3 ? -25000 : 0;
        input.poll(0,{},env,sink.adapter()); input.poll(10,{0,static_cast<std::int16_t>(x),static_cast<std::int16_t>(y)},env,sink.adapter());
        input.poll(30,{0x8000},env,sink.adapter()); input.poll(40,{},env,sink.adapter()); input.poll(10+gap,{0x8000},env,sink.adapter());
        check((sink.calls != 0) == (gap < 1000), "early direction and repeated button use the independent1000ms table");
        if (sink.calls) check(sink.last.variant == row+10, "early and established variants never share a guessed identity");
    }
}
void lip_and_geometry_readiness() {
    InputSession session; InputAdmission a{}; Sink sink;
    a.gameplay=true; a.observed_lip=true; a.pose=SkatePose::air; a.input_policy=InputPolicy::strict(); a.extensions=sink.adapter();
    a.rail.phase=InputRailPhase::lip_selection; a.rail.fresh=a.rail.ready=a.rail.geometry_known=true;
    a.rail.generation=7; a.rail.selection_frame=a.rail.gameframe=100;
    session.poll(0,{},0,0,a); session.poll(10,{0,-25000,25000},0,0,a);
    check(sink.calls==0,"lip selection waits for actual update counters before attempting native execution");
    a.rail.gameframe=101; session.poll(1000,{},0,0,a);
    check(sink.calls==0,"two gameframes are not replaced by elapsed wall-time sixtieths");
    a.rail.gameframe=102; auto out=session.poll(1001,{},0,0,a);
    check(sink.calls==1 && sink.last.variant==7 && out.unsupported && !out.phase,
        "eligible lip selection remains a request and unsupported adapter cannot fabricate a physical lip");
    session.poll(1100,{0,25000,25000},0,0,a);
    check(sink.calls==1,"selection table closes after its gameframe interval rather than accepting arbitrary later direction changes");
    InputSession idle; Sink default_sink; a.extensions=default_sink.adapter();a.rail.generation=8;a.rail.selection_frame=a.rail.gameframe=200;
    idle.poll(0,{},0,0,a);a.rail.gameframe=201;idle.poll(17,{},0,0,a);check(default_sink.calls==0,"no premature default lip");
    a.rail.gameframe=202;idle.poll(34,{},0,0,a);idle.poll(51,{},0,0,a);
    check(default_sink.calls==1 && default_sink.last.variant==0,"default lip is selected once after an empty selection interval");
    for(bool owner_changed:{false,true}) {
        InputSession rail;Sink target;InputAdmission r{};r.gameplay=true;r.pose=SkatePose::grind;r.input_policy=InputPolicy::strict();r.extensions=target.adapter();
        r.rail.phase=InputRailPhase::established;r.rail.fresh=r.rail.ready=true;r.rail.generation=1;
        rail.poll(0,{},0,0,r);rail.poll(10,{0x8000},0,0,r);rail.poll(20,{},0,0,r);rail.poll(30,{0x4000},0,0,r);
        check(target.calls==0,"missing geometry cannot become native execution readiness");
        r.rail.geometry_known=true;if(owner_changed)r.rail.generation=2;
        out=rail.poll(40,{},0,0,r);
        check(target.calls==(owner_changed?0u:1u) && !out.phase,"generation change cancels retained rail transition rather than applying it to a new contact");
    }
}
void lip_extra_tables() {
    struct Row {unsigned table,first,second,target,window;};
    constexpr Row rows[]{{0,13,13,40,300},{0,13,14,41,300},{0,14,14,42,300},{0,14,13,43,300},{0,15,15,44,300},
        {0,15,14,45,300},{0,15,13,46,300},{1,14,14,50,300},{1,15,15,51,300},{1,13,13,52,300},
        {1,13,14,53,300},{1,14,13,54,300},{1,15,13,55,300},{2,15,15,56,500}};
    for(const auto& row:rows)for(unsigned age:{row.window-1,row.window,row.window+1}){
        UnifiedInput input;Sink sink;InputEnvironment e{InputContext::lip,true};e.policy=InputPolicy::strict();
        e.rail.phase=InputRailPhase::lip_established;e.rail.fresh=e.rail.ready=true;e.rail.lip_table=row.table;
        input.poll(0,{},e,sink.adapter());input.poll(10,{static_cast<std::uint16_t>(1u<<row.first)},e,sink.adapter());input.poll(20,{},e,sink.adapter());
        input.poll(10+age,{static_cast<std::uint16_t>(1u<<row.second)},e,sink.adapter());
        check((sink.calls!=0)==(age<row.window),"lip posture tables preserve their own exact query windows");
        if(sink.calls)check(sink.last.action==InputAction::lip_change && sink.last.variant==row.target,"lip transitions retain context-specific targets without invented native identities");
    }
}
void wall_controls() {
    for(auto direction:{std::int16_t{2},std::int16_t{1},std::int16_t{3}})for(bool reverse:{false,true})for(unsigned age:{449u,450u,451u}){
        UnifiedInput input;Sink sink;InputEnvironment e{InputContext::air,true};e.policy=InputPolicy::strict();e.ollie_allowed=false;
        e.surface.fresh=e.surface.can_plant=true;e.surface.generation=3;
        input.poll(0,{},e,sink.adapter());e.surface.physics_step=1;
        const auto x=direction==1?-25000:direction==3?25000:0;
        const PhysicalInputSample down{0,static_cast<std::int16_t>(x),-25000};
        input.poll(10,reverse?PhysicalInputSample{0x1000}:down,e,sink.adapter());e.surface.physics_step=2;
        input.poll(10+age,reverse?down:PhysicalInputSample{0x1000},e,sink.adapter());
        check((sink.calls!=0)==(age<450),"all six plant orders use the source450ms window after geometry admission");
        if(sink.calls)check(sink.last.action==InputAction::wall_command && sink.last.variant==1 && sink.last.surface_generation==3,
            "wall recognition retains native surface provenance and never writes geometry or velocity");
    }
    UnifiedInput input;Sink sink;InputEnvironment e{InputContext::ground,true};e.policy=InputPolicy::strict();
    e.surface.fresh=e.surface.can_push=true;e.surface.generation=1;e.surface.physics_step=1;
    input.poll(0,{},e,sink.adapter());e.surface.physics_step=2;input.poll(10,{0x8000},e,sink.adapter());input.poll(11,{0x8000},e,sink.adapter());
    check(sink.calls==1 && sink.last.variant==2,"held push is evaluated once per actual physics step rather than every input read");
    e.surface.physics_step=3;input.poll(20,{0x8000},e,sink.adapter());check(sink.calls==2,"a new eligible physics step may retry an unsupported held command");
    e.surface.fresh=false;input.poll(30,{0x8000},e,sink.adapter());check(sink.calls==2,"unavailable wall context never invents an eligible surface");
}
void air_extra_tables() {
    struct Actor:Sink {
        static InputDisposition start(void* state,const InputRequest& request) noexcept {
            Sink::receive(state,request);return InputDisposition::started;
        }
        InputAdapter adapter() noexcept{return{this,start};}
    };
    UnifiedInput input;Actor actor;InputEnvironment e{InputContext::air,true};e.policy=InputPolicy::strict();e.nollie=true;
    input.poll(0,{},e,actor.adapter());input.poll(10,{0x4000,-25000,0},e,actor.adapter());
    input.poll(20,{},e,actor.adapter());e.nollie=false;input.poll(30,{0x4000},e,actor.adapter());
    check(actor.last.presses==2 && actor.last.direction==4 && actor.last.direction_inherited && actor.last.nollie,
        "extra flip inherits the admitted parent direction and early stance instead of fabricating a fresh modifier edge");
    input.poll(40,{},e,actor.adapter());input.poll(50,{0x4000},e,actor.adapter());
    check(actor.last.presses==3,"declared second extra advances the parent graph");
    const auto calls=actor.calls;input.poll(60,{},e,actor.adapter());input.poll(70,{0x4000},e,actor.adapter());
    check(actor.calls==calls,"a terminal parent cannot invent a fourth inherited action from a consumed released direction");
    input.poll(80,{},e,actor.adapter());input.poll(90,{0x4000,-25000,0},e,actor.adapter());
    check(actor.last.presses==1 && !actor.last.direction_inherited,"fresh direction starts an ordinary action after a terminal extra table");
}

void grab_release_and_reset() {
    struct Waiting {
        unsigned calls{}; bool ready{};
        static InputDisposition receive(void* state,const InputRequest&) noexcept {
            auto& self=*static_cast<Waiting*>(state);++self.calls;return self.ready?InputDisposition::started:InputDisposition::waiting;
        }
        InputAdapter adapter() noexcept{return {this,receive};}
    };
    UnifiedInput input;Waiting target;InputEnvironment e{InputContext::air,true};e.policy=InputPolicy::strict();
    input.poll(0,{},e,target.adapter());input.poll(10,{0x2000},e,target.adapter());
    const auto attempted=target.calls; input.poll(20,{},e,target.adapter());target.ready=true;input.poll(30,{},e,target.adapter());
    check(target.calls==attempted,"a released waiting hold cannot acquire ownership when readiness arrives later");
    input.poll(40,{0x2000},e,target.adapter());
    check(target.calls==attempted+1,"release followed by a genuine new press admits one new hold request");
    for(unsigned boundary=0;boundary<5;++boundary) {
        InputSession session;InputAdmission a{};a.gameplay=a.action_fresh=a.inspected_stance=true;a.pose=SkatePose::air;a.action_mode=ActionExperiment::classic;
        session.poll(0,{},0,0,a);check(session.poll(10,{0x2000},0,0,a).action.owned,"hold starts before cancellation boundary");
        if(boundary==0)a.gameplay=false;if(boundary==1)a.action_fresh=false;if(boundary==2)a.external_owner=true;
        if(boundary==3)a.action_mode=ActionExperiment::off;if(boundary==4)a.pose=SkatePose::bail;
        const auto out=session.poll(20,{0x2000},0,0,a);
        check(!out.action.owned && !out.phase,"OFF, freshness, owner and bail boundaries release all held adapter channels");
        a.gameplay=a.action_fresh=true;a.external_owner=false;a.action_mode=ActionExperiment::classic;a.pose=SkatePose::air;
        check(!session.poll(30,{0x2000},0,0,a).action.owned,"holding through a cancelled interval cannot become a new physical press");
        session.poll(40,{},0,0,a);check(session.poll(80,{0x2000},0,0,a).action.owned,"release rearms a real press after a cancellation boundary");
    }
}

}
int main() { grab_release_and_reset(); normalization(); sampled_windows(); preparation(); early_contact_readiness(); grab_hold(); rail_tables(); lip_and_geometry_readiness(); lip_extra_tables(); wall_controls(); air_extra_tables(); std::cout << checks << " control state checks passed\n"; }
