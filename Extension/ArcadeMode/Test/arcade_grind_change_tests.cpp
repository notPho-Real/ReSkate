#include "Extension/ArcadeMode/arcade_grind_change.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why) {
    ++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}
}
struct Pipeline {
    GrindChange change;
    ArcadeInput input{true};
    NativeInputState physics;
    NativeInputOwner owner{1,2,3,4,5};
    NativeInputFrame frame{};
    NativeGrindContact contact{};
    NativeGrindSnapshot animation{};
    std::uint64_t now{},arms{},last_deadline{};
    InputRequest dispatched{};
    std::vector<PreparedGesture> catalog{
        {1,false,GestureContext::from_grind,{{0,0,-28000},{18,0,28000},{21,0,0},{24,0,0}}}};
    Pipeline() {
        input.enable(true);
        contact.valid=true;contact.actor=16;contact.output=24;contact.state=401;contact.identity=20;contact.geometry=1;
        contact.begin={0,.49f,0};contact.end={0,.49f,-6};contact.segment_identity={7,8};
    }
    void step(std::uint64_t time,unsigned state=401) {
        now=time;NativeRequestSample native{};native.valid=true;native.state=state;native.crouch_mode=1;
        frame=physics.observe(now,owner,native,{true,-1,32});
        input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
        contact.state=state;
        animation.valid=animation.grind_type_valid=animation.physics_grind_type_valid=true;
        animation.physics_state=state;animation.sampled_at=now;
        animation.grind_type=animation.physics_grind_type=contact.identity;
    }
    NativeGesture::Output poll(std::uint16_t buttons=0,bool allowed=true) {
        change.observe(now,frame,contact,allowed && !(buttons&classic_a),animation);
        const InputAdapter adapter{this,[](void* value,const InputRequest& request) noexcept {
            auto& p=*static_cast<Pipeline*>(value);p.dispatched=request;
            return p.change.request(request,p.now,[&](auto,auto deadline) noexcept {
                ++p.arms;p.last_deadline=deadline;return true;
            });
        }};
        return input.poll(0,now,buttons,0,0,0,0,allowed,true,false,0,false,false,true,now/8,false,catalog,
            false,false,SkatePose::grind,false,false,false,change.active(),false,false,0,0,
            ActionExperiment::classic,true,true,false,{},change.admission(),adapter);
    }
    void establish() {step(1000);poll();step(1100);poll();}
};
}
int main() {
    for(unsigned hz:{30u,60u,120u})for(bool together:{false,true}) {
        Pipeline p;p.establish();p.step(1200);
        p.poll(together?0xa000:0x8000);
        if(!together){p.step(1230);p.poll(0xa000);}
        const auto start=p.now;
        check(p.arms==1 && p.change.active(),"production facade dispatches Y/B to native armer exactly once");
        check(p.dispatched.grind_target()==GrindChangeTarget::nosegrind && p.dispatched.first_input_at==1200 &&
            p.dispatched.last_input_at==start,"sequential and simultaneous recognition preserve original event times");
        check(p.last_deadline==start+1500,"native admission has a bounded lease");
        check(!p.poll(0xa000).rail.held,"named ownership retires ordinary Y actor");
        for(unsigned i=0;i<1200*hz/1000;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.step(start+elapsed,elapsed<150?401:elapsed<500?406:403);
            if(elapsed>=500)p.contact.identity=23;
            p.poll(0xa000);p.poll(0xa000);
            auto pad=p.change.sample(p.now);
            check(p.arms==1,"held buttons and repeated polls never replay the actor");
            check(pad.owns==(elapsed<1000),"bounded right-stick ownership uses elapsed time at every cadence");
            if(pad.owns)check(pad.y==32767 && pad.x==(elapsed<200?0:12000),"Up remains held across measured heading change");
            check(!p.change.evidence().physics_changed,"requested name never supplies native completion evidence");
        }
        NativeGrindCompletion done{true,401,403,1,{}};
        NativeGrindSnapshot snapshot{};snapshot.valid=snapshot.grind_type_valid=snapshot.physics_grind_type_valid=true;
        snapshot.physics_state=403;snapshot.sampled_at=p.now;snapshot.grind_type=20;snapshot.physics_grind_type=23;
        p.change.verify(p.now,done,p.contact,snapshot);
        check(p.change.evidence().completed && p.change.evidence().physics_changed && !p.change.evidence().animation_changed,
            "actual destination contact and native completion cannot stand in for animation change");
        snapshot.grind_type=23;p.change.verify(p.now,done,p.contact,snapshot);
        check(p.change.evidence().animation_changed && !p.change.active(),"independent matching animation retires completed bounded request");
    }
    for(unsigned fault=0;fault<4;++fault) {
        Pipeline p;p.establish();p.step(1200);p.poll(0x8000);p.step(1230);
        switch(fault) {
        case 0:p.animation.grind_type=23;break;
        case 1:p.animation.valid=false;break;
        case 2:p.animation.sampled_at=1200;break;
        case 3:p.animation.physics_grind_type=23;break;
        }
        p.poll(0xa000);
        check(p.change.active() && !p.change.evidence().source_animation_known,
            "missing or mismatched animation baseline does not manufacture an initial identity");
        p.step(1300,406);p.poll();p.step(1400,403);p.contact.identity=23;p.poll();
        NativeGrindSnapshot destination{};destination.valid=destination.grind_type_valid=destination.physics_grind_type_valid=true;
        destination.physics_state=403;destination.grind_type=destination.physics_grind_type=23;destination.sampled_at=p.now;
        p.change.verify(p.now,{true,401,403,1,{}},p.contact,destination);
        check(p.change.evidence().physics_changed && !p.change.evidence().animation_changed,
            "an unchanged or initially unknown animation cannot become a verified named change");
    }
    for(unsigned fault=0;fault<10;++fault) {
        Pipeline p;p.establish();p.step(1200);p.poll(0x8000);p.step(1230);p.poll(0xa000);
        check(p.change.active(),"negative case starts an admitted actor");
        p.step(1250,406);
        switch(fault) {
        case 0:p.poll(0xa000,false);break;
        case 1:p.poll(classic_a);break;
        case 2:p.frame.owner.rig=123;p.poll();break;
        case 3:p.frame.valid=false;p.poll();break;
        case 4:p.now+=151;p.poll();break;
        case 5:p.step(1250,701);p.poll();break;
        case 6:p.step(1250,100);p.poll();break;
        case 7:p.step(1250,403);p.contact.segment_identity[0]++;p.poll();break;
        case 8:p.step(1250,403);p.contact.valid=false;p.poll();break;
        case 9:p.step(1501,406);p.poll();break;
        }
        check(!p.change.active() && !p.change.sample(p.now).owns,"OFF, A, owner/stale/exit/landing/geometry loss cancels input");
        check(!p.change.evidence().animation_changed,"cancellation never acknowledges a grind");
    }
    {
        Pipeline p;p.establish();p.step(1200);p.poll(0x2000);p.step(1230);p.poll(0xa000);
        check(p.dispatched.grind_target()==GrindChangeTarget::nosebluntslide && p.arms==0,
            "distinct B then Y remains ordered and unsupported recipe never arms native actor");
    }
    for(auto age:{299u,300u,301u}) {
        Pipeline p;p.establish();p.step(1200);p.poll(0x8000);
        p.step(1230);p.poll();p.step(1330);p.poll();p.step(1430);p.poll();
        p.step(1200+age);p.poll(0x2000);
        check(p.arms==(age==299?1u:0u),"released sequential buttons obey the original strict 300ms recognition window");
        if(p.arms)check(p.dispatched.first_input_at==1200 && p.dispatched.last_input_at==1499,
            "released buttons retain physical timestamps rather than dispatch time");
    }
    {
        Pipeline p;p.step(1000);p.poll();p.now=1100;p.poll();
        check(!p.change.admission().ready,"polls cannot fabricate established original physics time");
        p.step(1100);p.contact.geometry=2;p.poll();
        check(!p.change.admission().ready,"unqualified native geometry cannot admit named controls");
    }
    {
        Pipeline p;p.establish();p.step(1200);p.poll(0x8000);p.step(1230);p.poll(0xa000);
        p.step(1260);const auto out=p.poll(classic_a);
        check(out.jump.setup_started && out.jump.from_rail && out.right_y==-28000,
            "A cancels named RS before the existing rail-exit gesture acquires its channel");
    }
    std::cout<<checks<<" grind change production composition checks passed\n";
}
