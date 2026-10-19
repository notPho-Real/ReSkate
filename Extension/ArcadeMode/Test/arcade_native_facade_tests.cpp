#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why) { ++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);} }
struct NativeFacade {
    ArcadeInput input{true,InputPolicy::strict()};
    std::uint64_t owner{1}; unsigned source{1};bool focus{true};
    NativeFacade(){input.enable(true);}
    NativeGesture::Output poll(std::uint64_t now,short y=0,unsigned user=0) {
        input.publish_authority(now,owner,source,focus);
        return input.poll(user,now,0,0,y,321,-456,true,false,false,0,false,false,false,now/17,false,{},false,true,SkatePose::riding);
    }
};
}
int main(){
    NativeFacade fresh;
    check(!fresh.input.admitted(0,1000),"production bound facade requires independently published authority");
    check(fresh.poll(1000).right_x==0&&fresh.input.admitted(0,1000),"validated local source enables the production session");
    const auto unbound=fresh.poll(1017,25000,1);
    check(unbound.right_x==321&&unbound.right_y==-456&&!fresh.input.busy(1),"unbound controller cannot own synthetic input and retains its physical packet");
    fresh.poll(1034,25000);
    check(fresh.poll(1051,-25000).manual.owns,"canonical production direction pair reaches native manual actuation");
    fresh.owner=2;
    check(!fresh.poll(1068,-25000).manual.owns&&!fresh.input.busy(0),"owner epoch change clears every session before held input baselines");
    fresh.poll(1085,0);fresh.poll(1102,25000);
    check(fresh.poll(1119,-25000).manual.owns,"new owner can issue a genuinely fresh sequence");
    fresh.source=3;
    check(!fresh.poll(1136,-25000).manual.owns,"connected source set change cannot inherit a manual or pending request");
    fresh.focus=false;check(!fresh.poll(1153).manual.owns&&!fresh.input.admitted(0,1153),"focus loss invalidates native authority");
    fresh.focus=true;check(!fresh.poll(1170,-25000).manual.owns,"focus restoration does not manufacture an edge from a held direction");
    fresh.input.publish_authority(1200,2,1,true);
    check(!fresh.input.admitted(0,1351),"a stopped client snapshot expires independently of physical poll count");
    fresh.input.invalidate_authority();check(!fresh.input.admitted(0,1201),"physics-owner takeover invalidates admission until the client republishes a coherent context");
    for(std::uint64_t age : {219u,220u,275u}) {
        NativeFacade strict;strict.poll(2000);strict.poll(2017,25000);
        check(strict.poll(2017+age,-25000).manual.owns==(age<220),"production reference query has an explicit strict220ms boundary");
    }
    NativeFacade compatibility;compatibility.input.set_input_policy(InputPolicy::production());
    compatibility.poll(3000);compatibility.poll(3017,25000);
    check(compatibility.poll(3292,-25000).manual.owns,"separately selected55ms sampling compatibility remains available without changing the reference constant");
    compatibility.input.set_input_policy(InputPolicy::strict());
    check(!compatibility.input.busy(0),"normalization policy changes cancel all synthetic state");
    NativeInputState observer;
    NativeRequestSample rail_sample{};rail_sample.valid=true;rail_sample.state=401;rail_sample.crouch_mode=1;
    ArcadeInput rail_input(true,InputPolicy::strict());rail_input.enable(true);
    auto rail_poll=[&](std::uint64_t now,short y) {
        const auto frame=observer.observe(now,{1,2,3,4,5},rail_sample,{true,-1});
        rail_input.publish_native_context(frame);rail_input.publish_authority(now,frame.owner_generation,1,true);
        return rail_input.poll(0,now,0,0,y,0,0,true,true,true,0,false,false,false,now/17,false,{},false,true,SkatePose::riding);
    };
    rail_poll(3500,0);rail_poll(3517,25000);
    check(!rail_poll(3534,-25000).manual.owns,"real401 rail context overrides stale grounded admission without fabricating Manual state");
    const auto published=rail_input.native_context(3534);
    check(published.valid&&published.rail_family&&published.physics_step==3&&published.contact_generation&&published.action_generation,
          "facade publishes correlated native generations and rail-family exclusion evidence");
    rail_input.publish_authority(3700,published.owner_generation,1,true);
    const auto stale=rail_input.poll(0,3700,0,0,0,321,-456,true,true);
    check(stale.right_x==321&&stale.right_y==-456&&!rail_input.busy(0),"fresh client authority cannot renew an expired native context");
    rail_input.publish_native_context({});
    check(!rail_input.native_context(3534).valid,"native observation loss explicitly removes exclusion evidence");
    NativeCharge charge;
    check(!charge.semantic_release(4100,true,false,false,4000,4100,true).sequence,"unadmitted physical release cannot create a native launch identity");
    const auto release=charge.semantic_release(4250,true,true,false,4000,4200,true);
    check(release.sequence==1&&release.ms==4200&&release.charge_ms==200,"native charge consumes original physical timestamps rather than readiness latency");
    check(charge.semantic_release(4267,true,true,false,4000,4200,true).sequence==1,"same semantic release cannot duplicate a launch");
    charge.cancel();
    check(!charge.semantic_release(4284,true,true,false,4000,4200,true).sequence,"cancellation cannot replay a consumed semantic release");
    check(!charge.semantic_release(4500,true,true,false,4000,4200,true).sequence,"stale release does not renew native execution lifetime");
    check(!charge.semantic_release(4600,true,true,true,4500,4600,true).sequence,"unsupported rail launch family cannot masquerade as an ordinary ground jump");
    ArcadeInput jumping(true,InputPolicy::strict());jumping.enable(true);
    const std::array prepared{PreparedGesture{1,false,GestureContext::ground,
        std::vector<AuthoredStickStep>{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}}};
    auto jump_poll=[&](std::uint64_t now,std::uint16_t buttons) {
        jumping.publish_authority(now,7,1,true);
        return jumping.poll(0,now,buttons,0,0,0,0,true,true,false,0,false,false,true,
            now/17,false,prepared,false,true,SkatePose::riding);
    };
    jump_poll(5000,0);
    check(jump_poll(5017,0x1000).authored_playback,"bound production facade admits an actual authored crouch setup");
    jump_poll(5200,0x1000);
    const auto edge=jump_poll(5267,0).jump;
    check(edge.released&&edge.setup_admitted&&edge.pressed_at==5017&&edge.released_at==5267,
          "actual input producer preserves physical press/release identity through the facade");
    NativeCharge producer;
    const auto token=producer.semantic_release(5300,edge.released,edge.setup_admitted,edge.from_rail,
        edge.pressed_at,edge.released_at,true);
    AirMotion physics;
    AirMotionIntent intent{true,true,5250,0,0,0,0,7};
    physics.step(intent,{5250,1,100,1.0f/60,5,2,0,true});
    intent={true,true,5300,token.sequence,token.ms,token.charge_ms,0,7};
    physics.step(intent,{5300,1,103,1.0f/60,5,2,0,true});
    intent.sampled_at=5317;
    const auto launch=physics.step(intent,{5317,1,200,1.0f/60,5,30,0,true});
    check(token.charge_ms==200&&launch.vertical&&std::abs(launch.velocity_y-13.43f)<.0001f,
          "physical release through real producer reaches native controller as one compensated charged launch");
    check(!jump_poll(5334,0).jump.released,"repeated production polls cannot manufacture another physical release");
    std::cout<<checks<<" production native facade checks passed\n";
}
