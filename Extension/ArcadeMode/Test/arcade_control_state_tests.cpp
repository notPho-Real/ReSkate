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
    for(int raw=-32768;raw<=32767;++raw){
        const auto state=InputEvents::normalize({0,static_cast<std::int16_t>(raw),static_cast<std::int16_t>(raw)},100);
        const int horizontal_byte=(raw+32767)/256,vertical_byte=(-raw+32767)/256;
        const int x=horizontal_byte<78?-1:horizontal_byte>178?1:0;
        const int y=vertical_byte<78?1:vertical_byte>178?-1:0;
        check(state.direction==InputEvents::encode(x,y),
            "every signed native axis value preserves independent byte thresholds after positive-up Y conversion");
    }
    for(bool nose:{false,true}){
        InputEvents events;events.sample(0,{});
        const std::int16_t threshold=nose?-13057:12800,neutral=nose?-13056:12799;
        events.sample(10,{0,0,neutral});check(events.history().events().empty(),"below-threshold travel is neutral and creates no cardinal event");
        events.sample(20,{0,0,threshold});const auto once=events.history().events().size();
        events.sample(30,{0,0,threshold});check(events.history().events().size()==once,"held sector has no repeated press or invented dwell");
        events.sample(40,{0,0,neutral});events.sample(50,{0,0,threshold});
        check(events.history().events().size()==3&&events.history().events()[0].at==20&&events.history().events()[1].at==40&&events.history().events()[2].at==50,
              "actual threshold crossings retain observed timestamps without recognition hysteresis or fabricated hardware time");
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
        env.policy=InputPolicy::compatibility();
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
    for (unsigned step : {8u,17u,34u}) for (const auto x : std::array<std::int16_t,3>{-25000,0,25000}) {
        UnifiedInput input; Sink sink; InputEnvironment e{InputContext::air,true}; e.ollie_allowed=false;
        e.surface={3,1,true,true,false,false,0};
        input.poll(0,{},e,sink.adapter());
        // A between physics steps must not be lost to the wall-push sampler.
        input.poll(step,{0x1000,x,0},e,sink.adapter());
        check(sink.calls==1 && sink.last.action==InputAction::wall_command && sink.last.variant==1 &&
            sink.last.surface_generation==3 && sink.last.first_input_at==step && sink.last.last_input_at==step,
            "fresh airborne A preserves its sole physical edge and contact provenance");
        e.surface.physics_step=2; input.poll(step*2,{0x1000,x,0},e,sink.adapter());
        check(sink.calls==1,"held A and fresh physics cannot repeat a wallplant");
        input.poll(step*3,{},e,sink.adapter());
        check(sink.calls==1,"wallplant release does not launch an Ollie");
    }
    for (unsigned missing=0;missing<6;++missing) {
        UnifiedInput input; Sink sink; InputEnvironment e{InputContext::air,true}; e.ollie_allowed=false;
        e.surface={3,1,true,true,false,false,0};
        input.poll(0,{},e,sink.adapter());
        if(missing==0)e.surface.fresh=false;
        if(missing==1)e.surface.can_plant=false;
        if(missing==2)e.surface.generation=0;
        if(missing==3)e.surface.physics_step=0;
        if(missing==4)e.context=InputContext::ground;
        if(missing==5)e.conflict=true;
        input.poll(10,{0x1000},e,sink.adapter());
        e.surface={3,2,true,true,false,false,0}; e.context=InputContext::air;e.conflict=false;
        input.poll(20,{0x1000},e,sink.adapter());
        check(sink.calls==0,"unqualified A cannot be promoted when a later wall becomes available");
    }
    UnifiedInput input;Sink sink;InputEnvironment e{InputContext::ground,true};e.policy=InputPolicy::strict();
    e.surface.fresh=e.surface.can_push=true;e.surface.generation=1;e.surface.physics_step=1;
    input.poll(0,{},e,sink.adapter());e.surface.physics_step=2;input.poll(10,{0x8000},e,sink.adapter());input.poll(11,{0x8000},e,sink.adapter());
    check(sink.calls==1 && sink.last.variant==2,"held push is evaluated once per actual physics step rather than every input read");
    e.surface.physics_step=3;input.poll(20,{0x8000},e,sink.adapter());check(sink.calls==2,"a new eligible physics step may retry an unsupported held command");
    e.surface.fresh=false;input.poll(30,{0x8000},e,sink.adapter());check(sink.calls==2,"unavailable wall context never invents an eligible surface");
}
void wallplant_contact_handoff() {
    struct Fixture {
        UnifiedInput input;Sink sink;
        InputEnvironment e{InputContext::air,true};
        Fixture() {
            e.ollie_allowed=false;
            e.native_context={true,true,{1,2,3},1,0,false,true,4,5};
        }
        void poll(std::uint64_t now,std::uint16_t buttons,bool surface=false,InputAdapter adapter={}) {
            e.native_context.sampled_at=now;++e.native_context.physics_step;
            if(surface)e.surface={7,e.native_context.physics_step,true,true,false,false,0,e.native_context.generations};
            input.poll(now,{buttons},e,adapter.request?adapter:sink.adapter());
        }
    };
    // The native collision callback must not require another physical pad poll.
    for (unsigned fault=0;fault<9;++fault) {
        Fixture f;f.poll(0,0);f.poll(10,0x1000);
        const auto press=f.input.history().events().back();
        const auto events=f.input.history().events().size();
        f.e.surface={7,f.e.native_context.physics_step,true,true,false,false,0,f.e.native_context.generations};
        if(fault==1)f.e.surface.can_plant=false;
        if(fault==2)f.e.gameplay=false;
        if(fault==3)f.e.conflict=true;
        if(fault==4)++f.e.native_context.generations.owner;
        if(fault==5)++f.e.native_context.airborne_episode;
        if(fault==6)f.e.native_context.airborne=false;
        if(fault==7)f.input.cancel_air_episode(true);
        const auto request=f.input.claim_wallplant_contact(fault==8?161:101,f.e);
        check((request.action==InputAction::wall_command)==(fault==0),
              "collision callback requires a real fresh journal A, eligible wall and unchanged airborne owner");
        if(!fault)check(request.input_sequence==press.sequence && request.physical_pressed_at==10,
                       "collision callback preserves the original A identity and press time");
        check(f.input.history().events().size()==events && f.sink.calls==0,
              "collision claim neither manufactures a pad event nor dispatches unrelated input");
        check(f.input.claim_wallplant_contact(fault==8?162:102,f.e).action==InputAction::none,
              "collision callback cannot consume the same press twice or revive rejected lineage");
    }
    for(const auto delay:{1u,34u,149u,150u,151u})for(bool released:{false,true}) {
        Fixture f;f.poll(0,0);f.poll(10,0x1000);
        const auto press=f.input.history().events().back();
        check(f.sink.calls==0 && press.owners==0,"airborne A remains unowned until independently eligible wall contact");
        if(released)f.poll(11,0);
        ++f.e.native_context.generations.contact;
        f.poll(10+delay,released?0:0x1000,true);
        check(f.sink.calls==(delay<=InputTiming::native_sample_lease_ms?1u:0u),
              "real airborne edge precedes contact only within the unchanged native lease, held or released");
        if(f.sink.calls) {
            const auto& request=f.sink.last;
            check(request.action==InputAction::wall_command && request.variant==1 &&
                  request.input_sequence==press.sequence && request.physical_pressed_at==10 &&
                  request.first_input_at==10 && request.last_input_at==10 && request.recognized_at==10+delay &&
                  request.air_episode==f.e.native_context.air_episode() && request.generations==f.e.native_context.generations,
                  "contact handoff preserves physical journal identity/time and binds the current native flight");
        }
        f.poll(20+delay,released?0:0x1000,true);
        check(f.sink.calls==(delay<=InputTiming::native_sample_lease_ms?1u:0u),
              "fresh contact publication cannot consume the same physical press twice or renew an expired edge");
    }
    for(unsigned fault=0;fault<13;++fault) {
        Fixture f;
        if(fault==0) {f.e.context=InputContext::ground;f.e.native_context.airborne=false;}
        if(fault==1)f.e.native_context.required=false;
        if(fault==2)f.e.native_context.rail_family=true;
        if(fault==3)f.e.conflict=true;
        if(fault==4)f.e.ollie_allowed=true; // Existing crouch consumes this same A.
        f.poll(0,fault==5?0x1000:0);
        f.poll(10,0x1000);
        const auto calls=f.sink.calls;
        if(fault==6)++f.e.native_context.generations.owner;
        if(fault==7)++f.e.native_context.airborne_episode;
        if(fault==8)++f.e.native_context.air_departure;
        if(fault==9)f.input.cancel_air_episode(true);
        if(fault==10)f.input.reset();
        if(fault==11){f.e.conflict=true;f.poll(20,0x1000);}
        if(fault==12){f.e.gameplay=false;f.poll(20,0x1000);}
        f.e.context=InputContext::air;f.e.native_context.airborne=true;f.e.native_context.required=true;
        f.e.native_context.rail_family=false;f.e.conflict=false;f.e.gameplay=true;f.e.ollie_allowed=false;
        f.poll(30,0x1000,true);
        check(f.sink.calls==calls,
              "ground, unproven, rail, conflicting, consumed, startup-held, changed-flight and cancelled A cannot be borrowed");
    }
    {
        Fixture f;f.poll(0,0);f.poll(10,0x1000);
        f.poll(20,0x5000); // Ordinary X owns its own request; it invalidates deferred A.
        const auto calls=f.sink.calls;
        f.poll(30,0x1000,true);
        check(calls==1 && f.sink.calls==calls && f.sink.last.action==InputAction::flip,
              "a competing recognized flip retains its ordinary request without admitting buffered wallplant A");
    }
    for(const auto button:{std::uint16_t{0x2000},std::uint16_t{0x4000}})
        for(const auto disposition:{InputDisposition::waiting,InputDisposition::started}) {
        struct Owner {
            InputDisposition disposition;unsigned tricks{},plants{};
            static InputDisposition receive(void* state,const InputRequest& request) noexcept {
                auto& self=*static_cast<Owner*>(state);
                if(request.action==InputAction::wall_command) {++self.plants;return InputDisposition::unsupported;}
                if(request.action==InputAction::flip || request.action==InputAction::grab) {
                    ++self.tricks;return self.disposition;
                }
                return InputDisposition::unsupported;
            }
        } owner{disposition};
        const InputAdapter adapter{&owner,Owner::receive};
        Fixture f;f.poll(0,0,false,adapter);f.poll(10,0x1000,false,adapter);
        f.poll(20,static_cast<std::uint16_t>(button|0x1000),false,adapter);
        check(owner.tricks==1 && owner.plants==0,"ordinary grab or flip adapter receives the competing request");
        f.e.conflict=disposition==InputDisposition::started;
        f.poll(30,0x1000,true,adapter);
        f.e.conflict=false;f.poll(40,0x1000,true,adapter);
        check(owner.plants==0,"queued grab/flip or observed active owner prevents deferred A from becoming a wallplant later");
    }
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
    check(target.calls==attempted+2,"release retains the request through both its waiting poll and later initialization poll");
    input.poll(40,{0x2000},e,target.adapter());
    check(target.calls==attempted+3,"release followed by a genuine new press admits a distinct semantic request");
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

void initial_rail_deadlines() {
    struct Waiting {
        unsigned calls{};InputRequest last{};
        static InputDisposition receive(void* state,const InputRequest& request) noexcept {
            auto& self=*static_cast<Waiting*>(state);++self.calls;self.last=request;return InputDisposition::waiting;
        }
        InputAdapter adapter() noexcept{return {this,receive};}
    };
    for(bool nose:{false,true})for(unsigned age:{399u,400u,401u}) {
        UnifiedInput input;Waiting target;InputEnvironment e{InputContext::ground,true};e.policy=InputPolicy::strict();
        const auto y=static_cast<std::int16_t>(nose?25000:-25000);
        input.poll(0,{},e,target.adapter());input.poll(10,{0,0,y},e,target.adapter());input.poll(20,{},e,target.adapter());
        input.poll(30,{0,0,y},e,target.adapter());input.poll(10+age,{0x8000,0,y},e,target.adapter());
        check((input.pending_rail().action==InputAction::rail_select)==(age<400),"initial repeated vertical rail query excludes the exact400ms boundary");
        if(age<400)check(input.pending_rail().variant==(nose?20:21),"initial rail selection remains a semantic family independent of native identity");
    }
    UnifiedInput input;Waiting target;InputEnvironment e{InputContext::air,true};e.policy=InputPolicy::strict();
    input.poll(0,{},e,target.adapter());input.poll(10,{0,0,25000},e,target.adapter());input.poll(20,{},e,target.adapter());
    input.poll(30,{0,0,25000},e,target.adapter());input.poll(40,{0x8000,0,25000},e,target.adapter());
    check(input.pending_rail().recognized_at==40 && input.pending_rail().deadline==1040,"rail request lifetime begins at recognition rather than the first query edge");
    e.context=InputContext::rail;input.poll(1040,{},e,target.adapter());
    check(input.pending_rail().action==InputAction::rail_select,"extra-grind pending lifetime includes its exact1000ms endpoint across contact");
    input.poll(1041,{},e,target.adapter());
    check(input.pending_rail().action==InputAction::none && input.diagnostic().dispatch[0].disposition==InputDisposition::expired,
        "extra-grind expires after1000ms without renewing on contact or readiness polls");
}

void extra_table_expiry() {
    struct Actor { unsigned calls{};InputRequest last{};
        static InputDisposition receive(void* state,const InputRequest& request) noexcept {
            auto& self=*static_cast<Actor*>(state);++self.calls;self.last=request;return InputDisposition::started;
        }
        InputAdapter adapter() noexcept{return{this,receive};}
    };
    for(bool grab:{false,true}) {
        const unsigned lifetime=grab?250:233;const auto face=static_cast<std::uint16_t>(grab?0x2000:0x4000);
        for(unsigned elapsed:{lifetime-1,lifetime,lifetime+1}) {
            UnifiedInput input;Actor actor;InputEnvironment e{InputContext::air,true};e.policy=InputPolicy::strict();e.named_grabs=grab;
            input.poll(0,{},e,actor.adapter());input.poll(10,{face,-25000,0},e,actor.adapter());input.poll(20,{},e,actor.adapter());
            input.poll(10+elapsed,{face},e,actor.adapter());
            check(elapsed<=lifetime ? actor.calls==2 && actor.last.presses==2 : actor.calls==1,
                "extra table includes its deadline; an expired named face waits without a directionless fallback");
            if(grab && elapsed>lifetime){
                input.poll(11+elapsed,{face,25000,0},e,actor.adapter());
                check(actor.calls==2&&actor.last.presses==1&&!actor.last.direction_inherited&&actor.last.direction==6&&
                    actor.last.physical_pressed_at==10+elapsed&&actor.last.recognized_at==11+elapsed,
                    "expired named-Grab face remains buffered for a genuine new direction with original physical press time");
            }
            if(elapsed<=lifetime)check(actor.last.presses==2 && actor.last.direction_inherited,
                "strict query window remains separate from inclusive extra table lifetime");
        }
    }
}

void prepared_pair_safety() {
    for(const auto first:{std::int16_t{25000},std::int16_t{-25000}}) {
        InputSession session;InputAdmission a{};a.gameplay=a.manual_prepare=true;a.pose=SkatePose::riding;
        session.poll(2000,{},0,0,a);session.poll(2010,{0,0,first},0,0,a);
        const auto opposite=static_cast<std::int16_t>(-first);
        check(!session.poll(2020,{0,0,opposite},0,0,a).manual.owns && session.pending_manual().recognized_at==2020,
            "the recovery pair is recognized while preparation alone cannot actuate");
        a.manual_safe=true;a.manual_prepare=false;const auto ready=session.poll(2030,{0,0,opposite},0,0,a);
        check(ready.manual.owns && ready.right_y==(first>0?-20000:20000) && session.pending_manual().action==InputAction::none,
            "fresh native safety dispatches the original recovery pair without a new physical edge");
        a.manual_safe=false;a.manual_prepare=true;
        check(!session.poll(2031,{},0,0,a).manual.owns,"preparation cannot retain an already active unsafe native owner");
    }
}

void composed_manual_deadline() {
    for(bool landed:{false,true})for(unsigned age:{699u,700u,701u})for(const auto first:{std::int16_t{25000},std::int16_t{-25000}}) {
        InputSession session;InputAdmission a{};a.gameplay=a.manual_prepare=true;a.pose=SkatePose::air;a.input_policy=InputPolicy::strict();
        session.poll(0,{},0,0,a);session.poll(10,{0,0,first},0,0,a);
        session.poll(110,{0,0,static_cast<std::int16_t>(-first)},0,0,a);
        check(session.pending_manual().recognized_at==110,"air intent timestamp is fixed before readiness");
        a.manual_prepare=false;a.manual_safe=true;if(landed)a.pose=SkatePose::riding;
        const auto out=session.poll(110+age,{},0,0,a);
        check(out.manual.owns==(age<=700),"composed native readiness admits at699/700ms and expires at701ms across landing");
        if(age<=700)check(out.manual.request_at==(landed?110+age:110),"landing settling clock is separate from the original airborne pending lifetime");
    }
}

void policy_and_mode_boundaries() {
    check(InputPolicy::production().manual_sample_allowance==55 && InputPolicy::production().inclusive_manual && InputPolicy::production().reject_opposing,
        "production preserves the ground sampling allowance and opposing-direction safety");
    check(InputPolicy::production().normalization==InputNormalization::analog &&
        InputPolicy::strict().manual_sample_allowance==0 && !InputPolicy::strict().inclusive_manual,
        "ground usability does not replace analog normalization or the separate strict reference policy");
    struct Waiting {
        unsigned calls{};
        static InputDisposition receive(void* state,const InputRequest&) noexcept {++static_cast<Waiting*>(state)->calls;return InputDisposition::waiting;}
        InputAdapter adapter() noexcept{return{this,receive};}
    } target;
    UnifiedInput input;InputEnvironment e{InputContext::ground,true};
    input.poll(0,{},e,target.adapter());input.poll(10,{0,0,25000},e,target.adapter());input.poll(110,{0,0,-25000},e,target.adapter());
    check(input.pending_manual().action==InputAction::manual,"precondition retains the original policy request");
    const auto session=input.diagnostic().session;const auto calls=target.calls;e.policy=InputPolicy::strict();
    input.poll(120,{0,0,-25000},e,target.adapter());
    check(input.pending_manual().action==InputAction::none && input.diagnostic().session>session && target.calls==calls && input.history().events().empty(),
        "policy changes cancel old requests and baseline held controls without synthesizing edges");
    check(input.diagnostic().policy==InputPolicy::strict(),"diagnostics disclose the actual installed policy");
    input.poll(130,{0,0,-25000},e,target.adapter());check(target.calls==calls,"held-through-policy-change is not a new press");
    for(bool policy_change:{false,true}) {
        InputSession native;InputAdmission a{};a.gameplay=a.action_fresh=a.inspected_stance=true;a.pose=SkatePose::air;a.action_mode=ActionExperiment::classic;
        native.poll(0,{},0,0,a);check(native.poll(10,{0x2000},0,0,a).action.owned,"hold starts before policy or adapter selector changes");
        if(policy_change)a.input_policy=InputPolicy::strict();else a.action_mode=ActionExperiment::revert_frontside;
        check(!native.poll(20,{0x2000},0,0,a).action.owned,"policy or adapter selection changes release the old native channels");
        a.action_mode=ActionExperiment::classic;
        check(!native.poll(30,{0x2000},0,0,a).action.owned,"returning to a selector cannot replay a held physical control");
        native.poll(40,{},0,0,a);check(native.poll(80,{0x2000},0,0,a).action.owned,"release rearms a fresh command under the selected policy");
    }
    InputSession authored;InputAdmission a{};const std::array catalog{PreparedGesture{1,false,GestureContext::ground,
        std::vector<AuthoredStickStep>{{0,0,-32767},{10,0,0}}}};
    a.catalog=catalog;a.gameplay=a.authored=a.may_start=true;a.pose=SkatePose::riding;
    authored.poll(0,{},0,0,a);check(authored.poll(10,{0x1000},0,0,a).authored_playback,"setup starts before policy replacement");
    a.input_policy=InputPolicy::strict();check(!authored.poll(20,{0x1000},0,0,a).authored_playback,"policy replacement cancels authored playback as well as the journal");
    check(!authored.poll(30,{},0,0,a).jump.released,"cancelled setup cannot emit a later launch token");
}
void state_bound_transition_requests() {
    struct Waiting {
        unsigned calls{};bool ready{};InputRequest last{};
        static InputDisposition receive(void* state,const InputRequest& request) noexcept {
            auto& self=*static_cast<Waiting*>(state);++self.calls;self.last=request;return self.ready?InputDisposition::started:InputDisposition::waiting;
        }
        InputAdapter adapter() noexcept{return{this,receive};}
    };
    for(unsigned invalidation=0;invalidation<4;++invalidation) {
        InputSession session;Waiting target;InputAdmission a{};a.gameplay=a.manual_balance_admitted=a.branch_ready=true;
        a.pose=SkatePose::manual;a.extensions=target.adapter();a.input_policy=InputPolicy::strict();
        session.poll(0,{},0,0,a);session.poll(10,{0x4000},0,0,a);session.poll(20,{},0,0,a);session.poll(100,{0x4000},0,0,a);
        check(target.calls==1 && target.last.balance_origin==0 && target.last.unbounded,"queued transition captures its independently supplied origin without an invented short expiry");
        if(invalidation==1)a.observed_balance_branch=2;if(invalidation==2)a.manual_balance_admitted=false;if(invalidation==3)a.branch_ready=false;
        target.ready=true;const auto out=session.poll(500,{},0,0,a);
        check(target.calls==(invalidation?1u:2u) && !out.phase && out.command_busy==(invalidation==0),
            "a waiting transition dispatches only in its original admitted ready state");
    }
    for(std::uint64_t generation:{0u,1u})for(bool replaced:{false,true}) {
        InputSession session;Waiting target;InputAdmission a{};a.gameplay=true;a.pose=SkatePose::grind;a.input_policy=InputPolicy::strict();a.extensions=target.adapter();
        a.rail.phase=InputRailPhase::established;a.rail.fresh=a.rail.ready=true;a.rail.generation=generation;
        session.poll(0,{},0,0,a);session.poll(10,{0x8000},0,0,a);session.poll(20,{},0,0,a);session.poll(30,{0x4000},0,0,a);
        check(target.calls==0,"native geometry is still required before a retained transition can execute");
        a.rail.geometry_known=true;if(replaced)++a.rail.generation;target.ready=true;session.poll(40,{},0,0,a);
        check(target.calls==(replaced?0u:1u),"generation zero is a bound rail origin and cannot act as a wildcard after contact replacement");
    }
}

}
int main() { policy_and_mode_boundaries(); state_bound_transition_requests(); composed_manual_deadline(); prepared_pair_safety(); extra_table_expiry(); initial_rail_deadlines(); grab_release_and_reset(); normalization(); sampled_windows(); preparation(); early_contact_readiness(); grab_hold(); rail_tables(); lip_and_geometry_readiness(); lip_extra_tables(); wall_controls(); wallplant_contact_handoff(); air_extra_tables(); std::cout << checks << " control state checks passed\n"; }
