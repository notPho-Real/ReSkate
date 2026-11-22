#include "Extension/ArcadeMode/arcade_vert_air.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Engine/Game/Abi/orientation_target.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
using namespace vert_math;
namespace {
unsigned checks{};
void check(bool value,const char* why) {++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
VertPatch patch(Vec point,Vec normal) {
    VertPatch p;p.valid=true;
    for(auto& s:p.samples){s.valid=true;s.point=point;s.query_normal=normal;s.body={7,8,9};s.world_owner.value=10;}
    return p;
}
VertObservation ground() {
    VertObservation o;o.native.valid=true;o.native.owner={1,2,3,4,5};o.native.owner_generation=1;o.native.contact_generation=1;o.native.action_generation=1;o.native.mode_generation=1;
    o.native.physics_step=2;o.native.sampled_at=1000;o.native.state=100;o.native.contacts=0xc000;
    o.native.physics_seconds=.01f;o.native.mode=1;o.position={0,5,0};o.velocity={2,8,0};
    o.up={-.9701425f,.2425356f,0};o.support=patch(o.position,o.up);o.supported=o.enabled=true;
    return o;
}
VertObservation fly(VertObservation o) {
    ++o.native.physics_step;o.native.sampled_at+=10;o.native.state=200;o.native.contacts=0;
    ++o.native.contact_generation;++o.native.action_generation;o.native.airborne=true;o.native.airborne_episode=1;o.native.airborne_started_known=true;
    o.native.airborne_started_at=o.native.sampled_at;o.supported=false;o.support={};return o;
}
void warm(VertAirController& c,VertObservation o) {
    o.native.sampled_at-=20;o.native.physics_step=1;o.position[1]-=.2f;
    o.up={-.8660254f,.5f,0};o.velocity={4,6.928203f,0};o.support=patch(o.position,o.up);
    c.observe(o);
}
void lifecycle() {
    auto o=ground();VertAirController c;warm(c,o);c.observe(o);check(c.phase()==VertPhase::rising_support,"steep support qualifies");
    o=fly(o);c.observe(o);check(c.eligible(o.native,1010),"genuine takeoff qualifies");
    check(!c.consume(o.native,1010,o.velocity),"no input never writes motion");
    check(!c.request(o.native,1010,1000,1),"pre-takeoff press cannot request transfer");
    for(unsigned i=0;i<4;++i){++o.native.physics_step;o.native.sampled_at+=10;o.velocity[1]-=.0981f;c.observe(o);}
    check(std::abs(c.gravity()-9.81f)<.01f,"gravity measured from native step cadence");
    check(c.request(o.native,1050,1050,2),"fresh air input accepted");
    check(!c.request(o.native,1050,1050,3),"same episode admits one request");
    auto plan=vert_level_out(o,c.direction());check(plan.action==VertAction::level_out,"no destination selects break vert");
    check(length(plan.velocity)<=length(o.velocity)+.001f && length(sub(plan.velocity,o.velocity))<=8.001f,"level out bounded without energy injection");
    check(!c.stage(plan,o,1050),"uncleared flight cannot stage");plan.clear=true;
    check(c.stage(plan,o,1050),"cleared flight staged");
    check(bool(c.consume(o.native,1050,o.velocity)),"one native physical impulse");
    check(!c.consume(o.native,1050,o.velocity),"duplicate physics call cannot repeat");
    auto landing=ground();landing.native.physics_step=8;landing.native.sampled_at=1070;landing.up={0,1,0};landing.support=patch({0,0,0},{0,1,0});
    c.observe(landing);check(c.phase()==VertPhase::landed,"actual supported riding establishes landing");
    for(unsigned negative=0;negative<7;++negative){
        c.reset();o=ground();warm(c,o);
        if(negative==0){o.support=patch(o.position,{0,1,0});o.up={0,1,0};}
        if(negative==1)o.supported=false;
        if(negative==2)o.native.state=401;
        if(negative==3)o.support.valid=false;
        if(negative==4)o.contested=true;
        c.observe(o);o=fly(o);
        if(negative==5)o.native.owner_generation++;
        if(negative==6)o.native.sampled_at+=151;
        c.observe(o);check(!c.eligible(o.native,o.native.sampled_at),"flat, rail, missing support, contested, stale owner and gap reject");
    }
    for(unsigned negative=0;negative<5;++negative){
        c.reset();o=ground();warm(c,o);c.observe(o);o=fly(o);c.observe(o);
        check(c.request(o.native,1010,1010,1),"request before cancellation");
        if(negative==0)o.enabled=false;
        if(negative==1)o.contested=true;
        if(negative==2)o.native.owner.core++;
        if(negative==3)o.native.valid=false;
        if(negative==4){o.native.airborne=false;o.native.state=500;o.supported=false;}
        ++o.native.physics_step;c.observe(o);
        check(!c.pending(o.native,1010) && c.phase()!=VertPhase::landed,"cancellation never fabricates landing");
    }
}
Vec rotate(Vec v,float a){return {v[0]*std::cos(a)-v[2]*std::sin(a),v[1],v[0]*std::sin(a)+v[2]*std::cos(a)};}
void destinations() {
    unsigned found{};
    for(float gravity:{9.81f,20.f,34.29f})for(float speed:{6.f,10.f,14.f})for(float angle:{0.f,.7f,1.6f,3.1f})for(float shift:{0.f,1000.f}) {
        auto o=ground();o.position={shift,6,shift};o.velocity=rotate({speed,3,0},angle);
        const auto origin=patch(o.position,rotate({-.98f,.1989975f,0},angle));
        for(float landing_time:{.8f,1.2f,1.6f}) {
            const auto point=position(o.position,o.velocity,gravity,landing_time);
            auto arrival=o.velocity;arrival[1]-=gravity*landing_time;
            if(arrival[1]>=-.5f)continue;
            // Surface tangent follows the descending ballistic approach, with
            // small positive normal impact. This constructs reachable ramps.
            auto normal=rotate({-arrival[1],speed,0},angle);unit(normal);
            const auto target=patch(point,normal);
            const auto plan=vert_destination(o,origin,target,rotate({1,0,0},angle),gravity,landing_time);
            if(normal[1]<.15f || normal[1]>.92f)continue;
            // Add a small approach angle instead of exact tangency.
            auto adjusted=normal;adjusted[1]+=.04f;unit(adjusted);
            const auto valid=vert_destination(o,origin,patch(point,adjusted),rotate({1,0,0},angle),gravity,landing_time);
            if(valid.action!=VertAction::none){
                ++found;check(valid.action==VertAction::spine,"opposing ramp is spine under translation and yaw");
                check(length(sub(position(o.position,valid.velocity,gravity,valid.seconds),valid.landing))<.001f,"analytic trajectory meets geometry");
                check(!valid,"geometric candidate alone is never clearance or a landing");
                auto invalid=target;invalid.valid=false;
                check(vert_destination(o,origin,invalid,rotate({1,0,0},angle),gravity,landing_time).action==VertAction::none,"missing patch rejected");
                check(vert_destination(o,origin,target,rotate({-1,0,0},angle),gravity,landing_time).action==VertAction::none,"ramp behind intent rejected");
                invalid=patch(add(point,{100,0,100}),adjusted);
                check(vert_destination(o,origin,invalid,rotate({1,0,0},angle),gravity,landing_time).action==VertAction::none,"unreachable destination rejected");
            }
            (void)plan;
        }
    }
    check(found>20,"parametric valid destination coverage");
    auto o=ground();o.position={0,6,0};o.velocity={8,0,0};
    auto target=patch(position(o.position,o.velocity,9.81f,1),{.7f,.7141428f,0});
    auto hip=vert_destination(o,patch(o.position,{0,.2f,-.9797959f}),target,{1,0,0},9.81f,1);
    check(hip.action==VertAction::hip,"adjacent transition uses hip action");
    o.velocity[1]=-1;target=patch(position(o.position,o.velocity,9.81f,1),{.7f,.7141428f,0});
    auto acid=vert_destination(o,patch(o.position,{-.9797959f,.2f,0}),target,{1,0,0},9.81f,1);
    check(acid.action==VertAction::acid_drop,"lower aligned transition uses acid drop");
}
void staged_races_and_orientation() {
    for(unsigned failure=0;failure<4;++failure) {
        VertAirController c;auto o=ground();warm(c,o);c.observe(o);o=fly(o);c.observe(o);
        check(c.request(o.native,1010,1010,1),"race request admitted");
        auto plan=vert_level_out(o,{1,0,0});plan.clear=true;
        check(c.stage(plan,o,1010),"race plan staged from immutable source");
        ++o.native.physics_step;o.native.sampled_at+=10;o.position=add(o.position,scale(o.velocity,.01f));
        if(failure==1)o.velocity[0]+=3;
        if(failure==2)o.position[0]+=1;
        if(failure==3)o.native.owner_generation++;
        c.observe(o);
        check(bool(c.consume(o.native,1020,o.velocity))==(failure==0),"collision, displacement or owner replacement revokes staged impulse");
    }
    for(float dt:{1.f/30,1.f/60,1.f/120})for(float yaw:{0.f,.8f,2.f}) {
        std::array<float,16> target{std::cos(yaw),0,-std::sin(yaw),0,0,1,0,0,std::sin(yaw),0,std::cos(yaw),0,3,4,5,1};
        const auto before=target;
        check(vert_align_target(target,{.8f,.6f,0},dt),"physical target tilts toward destination");
        check(dingosdk::game::orientation_target::basis_valid(target),"orientation stays orthonormal");
        check(dingosdk::game::orientation_target::error(before,target)<=2.5f*dt+.0001f,"orientation rate bounded at multiple frame rates");
        check(target[12]==3 && target[13]==4 && target[14]==5,"orientation never moves native position");
    }
}
void unchanged_motion() {
    AirMotion c;AirMotionIntent intent;intent.enabled=true;intent.sampled_at=1000;intent.generation=1;
    AirMotionFrame f;f.now=1000;f.owner=1;f.state=100;f.dt=.01f;f.ordinary_ground=true;
    (void)c.step(intent,f);f.state=200;f.velocity_y=8;f.native_vert_air=true;
    auto output=c.step(intent,f);check(!output.vertical && !output.horizontal,"qualified vert suppresses ordinary gravity and launch shaping");
}
struct Requests {unsigned count{};InputRequest last{};};
void session_trigger_ownership() {
    InputSession session;Requests requests;InputAdmission a;
    a.gameplay=true;a.pose=SkatePose::air;a.action_mode=ActionExperiment::classic;
    a.extensions={&requests,[](void* p,const InputRequest& r) noexcept {
        if(r.action!=InputAction::transfer_command)return InputDisposition::unsupported;
        auto& out=*static_cast<Requests*>(p);++out.count;out.last=r;return InputDisposition::started;
    }};
    auto frame=fly(ground()).native;
    a.native_context=InputNativeContext::from_frame(frame);
    a.surface={1,frame.physics_step,true,false,false,true,0,a.native_context.generations};
    a.tick=1;(void)session.poll(1010,{},0,0,a);
    ++frame.physics_step;frame.sampled_at=1020;a.native_context=InputNativeContext::from_frame(frame);a.tick=2;
    PhysicalInputSample pad;pad.lt=255;
    auto result=session.poll(1020,pad,0,0,a);
    check(result.transfer_triggers_owned && requests.count==1,"accepted LT owns native trigger channel");
    a.surface.can_transfer=false;++frame.physics_step;frame.sampled_at=1030;a.native_context=InputNativeContext::from_frame(frame);a.tick=3;
    result=session.poll(1030,pad,0,0,a);
    check(result.transfer_triggers_owned && requests.count==1,"trigger ownership persists after request while held");
    pad.lt=0;++frame.physics_step;frame.sampled_at=1040;a.native_context=InputNativeContext::from_frame(frame);a.tick=4;
    result=session.poll(1040,pad,0,0,a);check(!result.transfer_triggers_owned,"physical release retires transfer trigger ownership");
}
void held_transfer_intent() {
    UnifiedInput input;Requests received;
    InputAdapter adapter{&received,[](void* p,const InputRequest& r) noexcept {
        if(r.action==InputAction::transfer_command){auto& out=*static_cast<Requests*>(p);++out.count;out.last=r;}
        return InputDisposition::started;
    }};
    InputEnvironment e;e.gameplay=true;e.context=InputContext::air;
    auto native=fly(ground()).native;
    PhysicalInputSample pad;pad.buttons=8;
    for(unsigned i=0;i<30;++i) {
        native.sampled_at+=20;++native.physics_step;
        e.native_context=InputNativeContext::from_frame(native);
        e.surface={1,native.physics_step,true,false,false,true,0,e.native_context.generations};
        if(i==29)pad.rt=255;
        input.poll(native.sampled_at,pad,e,adapter);
    }
    check(received.count==1 && received.last.direction==InputEvents::encode(1,0),
        "transfer uses still-held direction after the trick sequence window expires");
}
void input_edges() {
    for(bool right:{false,true})for(bool held_before:{false,true}) {
        UnifiedInput input;Requests received;
        InputAdapter adapter{&received,[](void* p,const InputRequest& r) noexcept {
            auto& out=*static_cast<Requests*>(p);if(r.action==InputAction::transfer_command){++out.count;out.last=r;}
            return InputDisposition::started;
        }};
        InputEnvironment e;e.gameplay=true;e.context=InputContext::ground;
        auto native=ground().native;e.native_context=InputNativeContext::from_frame(native);
        PhysicalInputSample physical;
        input.poll(1000,physical,e,adapter);
        if(held_before){if(right)physical.rt=255;else physical.lt=255;}
        native.sampled_at=1005;++native.physics_step;e.native_context=InputNativeContext::from_frame(native);
        input.poll(1005,physical,e,adapter);
        native=fly(ground()).native;native.physics_step=4;e.context=InputContext::air;e.native_context=InputNativeContext::from_frame(native);
        e.surface.fresh=e.surface.can_transfer=true;e.surface.generation=1;e.surface.physics_step=native.physics_step;
        e.surface.generations=e.native_context.generations;
        input.poll(1010,physical,e,adapter);
        if(right)physical.rt=255;else physical.lt=255;
        native.sampled_at=1020;++native.physics_step;e.native_context=InputNativeContext::from_frame(native);
        e.surface.generations=e.native_context.generations;
        input.poll(1020,physical,e,adapter);
        check(received.count==(held_before?0u:1u),"only fresh airborne trigger recognizes transfer");
        input.poll(1021,physical,e,adapter);check(received.count==(held_before?0u:1u),"held trigger never repeats transfer");
        if(!held_before)check(received.last.input_sequence && received.last.physical_pressed_at==1020 && received.last.air_episode.number==1,"edge timestamp and native flight retained");
    }
}
}
int main(){lifecycle();destinations();staged_races_and_orientation();unchanged_motion();session_trigger_ownership();held_transfer_intent();input_edges();std::cout<<checks<<" vert checks passed\n";}
