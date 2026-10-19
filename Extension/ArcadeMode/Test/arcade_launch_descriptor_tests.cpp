#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_air_motion.h"
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{},failures{};
void check(bool value,const char* why){++checks;if(!value){++failures;if(failures<12)std::cerr<<"FAIL: "<<why<<'\n';}}
bool near(float a,float b){return std::abs(a-b)<.0001f;}
void policy(){
    GameplayProfile profile;
    for(unsigned ms:{0u,1u,17u,34u,50u,100u,150u,199u,200u,201u,500u}){
        const auto fraction=static_cast<float>(std::min(ms,200u))/200;
        const LaunchDescriptor ordinary=LaunchDescriptor::ordinary(),extended{LaunchFamily::extended,LaunchContext::flat,0};
        check(near(profile.launch_speed(ms,ordinary),(350+100*fraction)*.0254f),"ordinary absolute charge curve converts inches once");
        check(near(profile.launch_speed(ms,extended),(400+125*fraction)*.0254f),"extended physical height policy is distinct from actor/pose admission");
        check(near(profile.launch_speed(ms,{LaunchFamily::ordinary,LaunchContext::vertical,0}),(100+175*fraction)*.0254f),"vertical policy uses its separate selected charge range");
        check(near(profile.launch_speed(ms,{LaunchFamily::extended,LaunchContext::vertical,0}),(200+125*fraction)*.0254f),"vertical extended range does not borrow ordinary flat values");
    }
    for(std::uint64_t age:{332u,333u,334u}){
        check(LaunchDescriptor::late_ground_eligible(1000+age,1000,true)==(age<=333),"late ground policy ends only after333ms from qualified departure");
        check(!LaunchDescriptor::late_ground_eligible(1000+age,1000,false),"elapsed time alone cannot qualify a late ground launch");
    }
    check(!LaunchDescriptor::late_ground_eligible(999,1000,true),"late ground rejects a future/rolled-back origin");
    check(LaunchDescriptor::direction_preload_ms==160,"direction cleanup policy uses160ms independent of the late launch gate");
    check(near(profile.launch_speed(0,{LaunchFamily::ordinary,LaunchContext::explicit_speed,12}),12),"explicit speed remains an absolute separately qualified policy");
}
void release_transport_writer(){
    const std::array catalog{PreparedGesture{
        1,false,GestureContext::ground,{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}},PreparedGesture{
        16,true,GestureContext::ground,{{0,0,32767},{18,25000,-28000},{21,0,0},{24,0,0}}}};
    for(unsigned cadence:{17u,34u})for(auto pose:{SkatePose::riding,SkatePose::manual,SkatePose::nose_manual})
    for(unsigned duration:{0u,1u,17u,34u,50u,100u,150u,199u,200u,201u,500u}){
        InputSession session;NativeCharge charge;AirMotion writer;AtomicLaunchDescriptor channel;
        InputAdmission admission{};admission.gameplay=admission.may_start=admission.authored=admission.manual_safe=true;
        admission.pose=pose;admission.catalog=catalog;admission.manual_balance_admitted=pose!=SkatePose::riding;
        const auto poll=[&](std::uint64_t now,bool held){
            admission.tick=now/17;
            return session.poll(now,{static_cast<std::uint16_t>(held?0x1000:0)},0,0,admission);
        };
        AirMotionIntent intent{true,false,1000};AirMotionFrame frame{1000,1,100,.001f*static_cast<float>(cadence),0,0,0,true};
        const auto physics=[&](std::uint64_t now,unsigned state){intent.sampled_at=frame.now=now;frame.state=state;return writer.step(intent,frame);};
        poll(1000,false);physics(1000,100);
        const auto pressed=1017u,released=pressed+duration;const auto setup=poll(pressed,true);physics(pressed,103);
        check(setup.jump.setup_started&&!setup.jump.released&&!setup.jump.setup_admitted,"actual admitted setup publishes a descriptor without pretending physical release occurred");
        charge.admit_setup(pressed,setup.jump.pressed_at,setup.jump.launch,setup.jump.setup_started);
        for(std::uint64_t now=pressed+cadence;now<released;now+=cadence){poll(now,true);physics(now,103);}
        // A later policy change cannot relabel already admitted setup.
        admission.launch={LaunchFamily::extended,LaunchContext::flat,0};
        const auto out=poll(released,false);
        check(out.jump.released&&out.jump.setup_admitted&&out.jump.launch==LaunchDescriptor::ordinary(),"actual ordinary/Manual/Nose setup freezes its descriptor through physical release");
        check(out.jump.pressed_at==pressed&&out.jump.released_at==released,"physical timestamp identity survives polling and modifier changes");
        const auto release=charge.semantic_release(released,out.jump.released,out.jump.setup_admitted,out.jump.from_rail,
            out.jump.pressed_at,out.jump.released_at,true,out.jump.launch);
        check(release.sequence==1&&release.ms==released&&release.charge_ms==std::min(duration,200u),"semantic charge carries the admitted descriptor with one capped physical release token");
        channel.store(release.launch);intent.launch=channel.load();intent.release_sequence=release.sequence;intent.release_at=release.ms;intent.charge_ms=release.charge_ms;
        physics(released,103);frame.velocity_y=30;const auto output=physics(released+cadence,200);
        const float expected=(350+100*static_cast<float>(std::min(duration,200u))/200)*.0254f;
        check(output.vertical&&near(output.velocity_y,expected),"descriptor read by owned writer selects physical charge without stacking native launch impulse");
        std::array<std::array<float,3>,2> velocities{{{3,30,4},{5,31,6}}},spins{};
        check(AirMotion::apply_bodies(output,velocities,spins)&&near(velocities[0][1],expected)&&near(velocities[1][1],expected+1),"existing body writer preserves articulation while applying captured absolute launch");
        check(near(AirMotion::next_height(0,output,frame.dt),expected*frame.dt-.5f*AirMotion::ordinary_gravity*frame.dt*frame.dt),"same descriptor-driven velocity feeds next-step displacement");
        const auto duplicate=charge.semantic_release(released+cadence,true,true,false,pressed,released,true,out.jump.launch);
        check(duplicate.sequence==release.sequence&&duplicate.ms==release.ms&&duplicate.launch==release.launch,"duplicate release cannot renew charge, descriptor or sequence");
    }
}
void unavailable(){
    const LaunchDescriptor descriptors[]{{},{LaunchFamily::extended,LaunchContext::flat,0},
        {LaunchFamily::ordinary,LaunchContext::vertical,0},{LaunchFamily::ordinary,LaunchContext::rail,0},
        {LaunchFamily::ordinary,LaunchContext::inverted,0},{LaunchFamily::ordinary,LaunchContext::explicit_speed,12},
        {static_cast<LaunchFamily>(255),LaunchContext::flat,0},{LaunchFamily::ordinary,static_cast<LaunchContext>(255),0},
        {LaunchFamily::ordinary,LaunchContext::flat,std::numeric_limits<float>::infinity()}};
    for(const auto descriptor:descriptors){
        AtomicLaunchDescriptor transport;transport.store(descriptor);
        check(transport.load().ordinary_writer()==descriptor.ordinary_writer(),"atomic descriptor transport preserves unsupported policy instead of defaulting its family");
        NativeCharge charge;
        check(!charge.semantic_release(1100,true,true,false,1000,1100,true,descriptor).sequence,"unknown policy or unsupported actor/context cannot become an ordinary native charge");
        AirMotion writer;AirMotionIntent intent{true,false,1000};AirMotionFrame frame{1000,1,100,.017f,0,0,0,true};writer.step(intent,frame);
        intent.launch=descriptor;intent.sampled_at=frame.now=1017;intent.release_at=1017;intent.release_sequence=1;intent.charge_ms=200;writer.step(intent,frame);
        intent.sampled_at=frame.now=1034;frame.state=200;frame.velocity_y=30;const auto out=writer.step(intent,frame);
        check(!out.vertical&&!out.horizontal,"unsupported descriptor preserves native takeoff velocity instead of falling back to an ordinary impulse");
    }
    AtomicLaunchDescriptor corrupt;corrupt.kind.store(0x10000u);check(!corrupt.load().known(),"noncanonical transport bits fail closed");
    for(const auto descriptor:descriptors){
        InputSession session;InputAdmission admission{};admission.gameplay=admission.may_start=admission.authored=true;
        admission.pose=SkatePose::riding;admission.launch=descriptor;
        const std::array catalog{PreparedGesture{1,false,GestureContext::ground,{{0,0,-32767},{18,25000,28000},{21,0,0},{24,0,0}}}};
        admission.catalog=catalog;session.poll(0,{},0,0,admission);
        const auto setup=session.poll(10,{0x1000},0,0,admission);const auto release=session.poll(20,{},0,0,admission);
        check(!setup.jump.setup_started&&!setup.requested_id&&setup.unsupported&&!release.jump.setup_admitted,
            "unsupported setup descriptor cannot borrow the ordinary authored actor or publish launch admission");
    }
    for(unsigned fault=0;fault<4;++fault){
        NativeCharge charge;
        const auto release=charge.semantic_release(1100,true,fault!=0,fault==1,1000,1100,fault!=2,LaunchDescriptor::ordinary());
        check((release.sequence!=0)==(fault==3),"missing setup, rail origin and lost eligibility cannot create a physical release");
    }
    NativeCharge charge;charge.poll(1000,true,true,true);
    const auto captured=charge.poll(1200,false,false,true,{});
    check(captured.launch==LaunchDescriptor::ordinary(),"legacy accepted setup captures its descriptor once rather than reading release-time configuration");
    charge.cancel();check(!charge.semantic_release(1400,true,true,false,1300,1400,true,{}).sequence,"unknown family cannot reuse an earlier valid released descriptor");
    check(!charge.semantic_release(1417,true,true,false,1300,1400,true,LaunchDescriptor::ordinary()).sequence,
        "a denied physical release cannot become a new charge by changing its descriptor later");
    NativeCharge denied;denied.semantic_release(1100,true,false,false,1000,1100,true);
    check(!denied.semantic_release(1117,true,true,false,1000,1100,true).sequence,
        "late setup admission cannot promote an already unadmitted physical release");
    NativeCharge future;future.semantic_release(1000,true,true,false,1000,1100,true);
    check(!future.semantic_release(1100,true,true,false,1000,1100,true).sequence,
        "a malformed future release cannot become fresh merely by waiting for its timestamp");
    for(unsigned fault=0;fault<5;++fault){
        NativeCharge state;state.semantic_release(1100,true,true,false,1000,1100,true);
        const auto at=fault==0?999u:fault==1?1200u:1100u;
        const auto now=fault==2?1251u:1100u;
        if(fault==3)state.admit_setup(1100,1100,LaunchDescriptor::ordinary(),true);
        const auto result=state.semantic_release(now,true,true,false,1000,at,true,
            fault==4?LaunchDescriptor{}:LaunchDescriptor::ordinary());
        check(!result.sequence,"malformed, stale, replaced or unknown release cannot return a preceding valid charge as fallback");
    }
    AirMotion writer;AirMotionIntent intent{true,false,1000};AirMotionFrame frame{1000,1,100,.017f,0,0,0,true};writer.step(intent,frame);
    intent.sampled_at=frame.now=1017;intent.release_sequence=1;intent.release_at=1017;writer.step(intent,frame);
    intent.sampled_at=frame.now=1034;intent.release_sequence=0;writer.step(intent,frame);
    intent.sampled_at=frame.now=1051;frame.state=200;frame.velocity_y=30;
    check(near(writer.step(intent,frame).velocity_y,30),"new setup publication withdraws a preceding unexecuted release before native takeoff");
    writer.reset();frame.state=100;frame.ordinary_ground=true;intent.sampled_at=frame.now=1100;writer.step(intent,frame);
    intent.sampled_at=frame.now=1117;intent.release_sequence=2;intent.release_at=1117;writer.step(intent,frame);
    intent.sampled_at=frame.now=1134;frame.state=200;frame.ordinary_ground=false;
    check(!writer.step(intent,frame).vertical,"loss of qualified ordinary context preserves native nonordinary impulse");
    AirMotionOutput cached{true,true,false,10};AirMotionIntent current{true,false,1000,1};
    const NativeAirSpinProof proof{{1,2,3,4,5},1,1,0,1,1000,true,true};
    check(AirMotion::vertical_target_current(cached,current,proof,proof,1000,10,.017f),"current captured ordinary policy retains the established target guard");
    current.launch={};
    check(!AirMotion::vertical_target_current(cached,current,proof,proof,1000,10,.017f),"cached target cannot restore vertical ownership through an unknown current descriptor");
}
void landing_rearms_ordinary_flight(){
    for(unsigned cadence:{17u,34u})for(unsigned ground:{100u,103u}){
        AirMotion writer;AirMotionIntent intent{true,false,1000};
        AirMotionFrame frame{1000,1,ground,.001f*static_cast<float>(cadence),0,0,0,true};
        const auto step=[&](unsigned state,bool ordinary,float velocity){
            intent.sampled_at=frame.now+=cadence;frame.state=state;frame.ordinary_ground=ordinary;frame.velocity_y=velocity;
            return writer.step(intent,frame);
        };
        writer.step(intent,frame);
        check(!step(200,false,-1).vertical,"unqualified departure withdraws vertical ownership");
        check(!step(201,true,-1).vertical,"restored basis within the same flight cannot rearm vertical ownership");
        step(ground,false,0);
        check(!step(200,true,-1).vertical,"unqualified ground cannot rearm a later flight");
        step(ground,true,0);
        const auto ledge=step(200,true,-1);
        check(ledge.vertical&&near(ledge.velocity_y,-1),"qualified ordinary landing rearms release-free ledge motion from observed native velocity");
        const auto continued=step(201,true,ledge.velocity_y);
        check(continued.vertical&&near(continued.velocity_y,-1-AirMotion::ordinary_gravity*frame.dt),"rearmed ledge continues ordinary gravity without adding charged impulse");
        const NativeAirSpinProof proof{{1,2,3,4,5},1,1,0,1,frame.now,true,true};
        check(AirMotion::vertical_target_current(continued,intent,proof,proof,frame.now,continued.velocity_y,frame.dt),
            "rearmed current-flight motion retains the existing cached target guard");
        step(ground,true,0);intent.release_sequence=1;intent.release_at=frame.now;intent.charge_ms=500;
        step(ground,true,0);const auto charged=step(200,true,30);
        check(charged.vertical&&near(charged.velocity_y,GameplayProfile{}.jump_speed(200)),"fresh ordinary release after recovery applies its original capped charge once");
        step(ground,true,0);const auto reused=step(200,true,-1);
        check(reused.vertical&&near(reused.velocity_y,-1),"landing cannot replay an already consumed charge token");
    }
    for(unsigned cadence:{17u,34u})for(const auto unsupported:{LaunchDescriptor{},
        LaunchDescriptor{LaunchFamily::extended,LaunchContext::flat,0},
        LaunchDescriptor{LaunchFamily::ordinary,LaunchContext::rail,0}}){
        AirMotion writer;AirMotionIntent intent{true,false,1000};
        AirMotionFrame frame{1000,1,100,.001f*static_cast<float>(cadence),0,0,0,true};writer.step(intent,frame);
        const auto step=[&](unsigned state){intent.sampled_at=frame.now+=cadence;frame.state=state;frame.velocity_y=-1;return writer.step(intent,frame);};
        intent.release_sequence=1;intent.release_at=1000;intent.charge_ms=200;intent.launch=unsupported;step(100);
        check(!step(200).vertical,"unsupported nonzero release cannot borrow ordinary launch motion");
        step(100);
        check(!step(200).vertical,"ordinary landing cannot promote a repeated unsupported release token");
        intent.release_sequence=0;step(100);const auto ledge=step(200);
        check(ledge.vertical&&near(ledge.velocity_y,-1),"withdrawn unsupported intent permits a later qualified release-free ordinary ledge");
    }
}

}
int main(){policy();release_transport_writer();unavailable();landing_rearms_ordinary_flight();std::cout<<checks<<" launch descriptor checks, "<<failures<<" failed\n";return failures?1:0;}
