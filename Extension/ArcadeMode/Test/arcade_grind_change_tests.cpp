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
    GrindAdmissionControl control{};
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
            return p.change.request(request,p.now,[&](auto,auto deadline,auto admission_control) noexcept {
                ++p.arms;p.last_deadline=deadline;p.control=admission_control;return true;
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
    for(unsigned hz:{30u,60u,120u})for(auto target:{GrindChangeTarget::nosegrind,GrindChangeTarget::crooked,GrindChangeTarget::grind_5_0})for(bool together:{false,true}) {
        const bool crooked=target==GrindChangeTarget::crooked;
        const bool tail=target==GrindChangeTarget::grind_5_0;
        // B then B requires two edges; simultaneous X+B has the existing
        // B->X Bluntslide priority and is deliberately not a 5-0 alias.
        if((crooked || tail) && together)continue;
        const unsigned identity=tail?24u:crooked?32u:23u;
        const std::uint16_t held=tail?0x6000u:crooked?0x2000u:0xa000u;
        Pipeline p;p.establish();p.step(1200);
        p.poll(tail?0x4000:crooked?0x2000:together?0xa000:0x8000);
        if(crooked){p.step(1210);p.poll();}
        if(!together){p.step(1230);p.poll(held);}
        const auto start=p.now;
        check(p.arms==1 && p.change.active(),"production facade dispatches qualified named binding exactly once");
        check(p.dispatched.grind_target()==target && p.dispatched.first_input_at==1200 &&
            p.dispatched.last_input_at==start,"sequential and simultaneous recognition preserve original event times");
        check(p.last_deadline==start+1500,"native admission has a bounded lease");
        check(p.control==GrindAdmissionControl::ordinary,"existing vertical recipes retain ordinary admission");
        check(!p.poll(held).rail.held,"named ownership retires ordinary Y actor");
        for(unsigned i=0;i<1200*hz/1000;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.step(start+elapsed,elapsed<150?401:elapsed<500?406:403);
            if(elapsed>=500)p.contact.identity=identity;
            p.poll(held);p.poll(held);
            auto pad=p.change.sample(p.now);
            check(p.arms==1,"held buttons and repeated polls never replay the actor");
            check(pad.owns==(elapsed<1000),"bounded right-stick ownership uses elapsed time at every cadence");
            if(pad.owns)check(pad.y==(tail?-32767:32767) && pad.x==(elapsed<200?0:crooked?24000:12000),"measured vertical input remains held across heading change");
            check(!p.change.evidence().physics_changed,"requested name never supplies native completion evidence");
        }
        NativeGrindCompletion done{true,401,403,1,{}};
        NativeGrindSnapshot snapshot{};snapshot.valid=snapshot.grind_type_valid=snapshot.physics_grind_type_valid=true;
        snapshot.physics_state=403;snapshot.sampled_at=p.now;snapshot.grind_type=20;snapshot.physics_grind_type=identity;
        auto other=p.contact;other.identity=identity==23?32u:23u;
        p.change.verify(p.now,done,other,snapshot);
        check(!p.change.evidence().physics_changed,"another qualified native name cannot satisfy the requested target");
        p.change.verify(p.now,done,p.contact,snapshot);
        check(p.change.evidence().completed && p.change.evidence().physics_changed && !p.change.evidence().animation_changed,
            "actual destination contact and native completion cannot stand in for animation change");
        snapshot.grind_type=identity;p.change.verify(p.now,done,p.contact,snapshot);
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
    for(bool tail:{false,true})for(unsigned fault=0;fault<10;++fault) {
        Pipeline p;p.establish();p.step(1200);p.poll(tail?0x4000:0x8000);p.step(1230);p.poll(tail?0x6000:0xa000);
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
        Pipeline p;p.establish();p.step(1200);p.poll(0x6000);
        check(p.dispatched.grind_target()==GrindChangeTarget::bluntslide && p.arms==0,
            "simultaneous X+B retains existing Bluntslide priority instead of dispatching the 5-0 recipe");
    }
    {
        Pipeline p;p.establish();p.step(1200);p.poll(0x2000);p.step(1230);p.poll(0xa000);
        check(p.dispatched.grind_target()==GrindChangeTarget::nosebluntslide && p.arms==0,
            "distinct B then Y remains ordered and unsupported recipe never arms native actor");
    }
    for(bool tail:{false,true})for(auto age:{299u,300u,301u}) {
        Pipeline p;p.establish();p.step(1200);p.poll(tail?0x4000:0x8000);
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
    for(unsigned hz:{30u,60u,120u}) {
        Pipeline p;p.contact.end[2]=-12;p.contact.identity=24;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();
        check(p.change.admission().ready && p.change.admission().excluded_variant==
            grind_change_variant(GrindChangeTarget::grind_5_0),"verified 5-0 admits return and excludes unchanged 5-0");
        p.step(1200,403);p.poll(0x8000);p.step(1210,403);p.poll();p.step(1230,403);p.poll(0x8000);
        check(p.arms==1 && p.change.active() && p.dispatched.grind_target()==GrindChangeTarget::grind_50_50 &&
            p.dispatched.first_input_at==1200 && p.dispatched.last_input_at==1230,"Y then Y dispatches a fresh native return lease");
        for(unsigned i=0;i<=1100*hz/1000;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.step(1230+elapsed,elapsed<50?403:elapsed<1000?406:401);
            if(elapsed>=1030)p.contact.identity=20;
            p.poll(0x8000);p.poll(0x8000);
            const auto pad=p.change.sample(p.now);
            check(pad.owns && pad.x==0 && pad.y==0,"return owns neutral through the slower original blend and identity publication");
            check(p.arms==1 && !p.change.evidence().physics_changed,"held Y and raw contact never manufacture completion");
        }
        auto snapshot=p.animation;snapshot.physics_state=401;snapshot.physics_grind_type=20;snapshot.grind_type=24;
        p.change.verify(p.now,{true,401,403,1,{}},p.contact,snapshot);
        check(!p.change.evidence().completed,"first-leg completion cannot satisfy the independently armed return");
        auto wrong=p.contact;wrong.segment_identity[0]++;
        p.change.verify(p.now,{true,403,401,1,{}},wrong,snapshot);
        check(!p.change.evidence().physics_changed,"return completion without original segment is not accepted");
        p.change.verify(p.now,{true,403,401,1,{}},p.contact,snapshot);
        check(p.change.evidence().physics_changed && !p.change.evidence().animation_changed && p.change.active(),
            "real return physics cannot stand in for a changed animation");
        snapshot.grind_type=20;p.change.verify(p.now,{true,403,401,1,{}},p.contact,snapshot);
        check(p.change.evidence().animation_changed && !p.change.active(),"independent return animation closes its lease");
        // Reacquire the returned native source using new physics samples;
        // holding the old Y may not replay either request.
        p.step(2400);p.poll(0x8000);p.step(2500);p.poll(0x8000);
        check(p.arms==1,"continuous held return button cannot replay after actor completion");
        p.step(2510);p.poll();p.step(2600);p.poll(0x8000);p.step(2630);p.poll(0xa000);
        check(p.arms==2 && p.change.active() && p.dispatched.grind_target()==GrindChangeTarget::nosegrind,
            "new physical pair after native return dispatches the existing Nosegrind executor");
    }
    for(unsigned fault=0;fault<5;++fault) {
        Pipeline p;p.contact.identity=24;p.contact.end[2]=-12;
        if(fault==0)p.contact.end[2]=-6;
        if(fault==1)p.contact.identity=23;
        if(fault==2)p.contact.identity=32;
        if(fault==3)p.contact.geometry=2;
        if(fault==4)p.contact.begin[1]=1;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();
        check(!p.change.admission().ready,"unqualified return source or geometry never arms the native gate");
    }
    for(unsigned fault=0;fault<5;++fault) {
        Pipeline p;p.contact.identity=24;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();p.step(1200,403);p.poll(0x8000);
        p.step(1210,403);p.poll();p.step(1230,403);p.poll(0x8000);check(p.change.active(),"negative return case armed");
        p.step(1300,406);
        switch(fault) {
        case 0:p.poll(classic_a);break;
        case 1:p.poll(0,false);break;
        case 2:p.frame.owner.core++;p.poll();break;
        case 3:p.step(1300,701);p.poll();break;
        case 4:p.now=2730;p.poll();break;
        }
        check(!p.change.active() && !p.change.sample(p.now).owns,"return neutral cancels on A, OFF, owner loss, exit or expiry");
    }
    for(unsigned hz:{30u,60u,120u})for(bool together:{false,true}) {
        Pipeline p;p.contact.identity=24;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();
        p.step(1200,403);p.poll(together?0x6000:0x2000);
        if(!together){p.step(1230,403);p.poll(0x6000);}
        const auto start=p.now;
        check(p.change.active() && p.arms==1 && p.control==GrindAdmissionControl::lateral_tail,
            "sequential B-X and simultaneous B+X dispatch only the measured lateral lease from 5-0");
        check(p.dispatched.grind_target()==GrindChangeTarget::bluntslide && p.dispatched.first_input_at==1200 &&
            p.dispatched.last_input_at==start,"lateral dispatch preserves physical event timestamps");
        for(unsigned i=0;i<=1100*hz/1000;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.contact.identity=elapsed<700?24u:elapsed<800?59u:31u;
            p.step(start+elapsed,elapsed<100?403:elapsed<700?406:402);p.poll(0x6000);p.poll(0x6000);
            const auto pad=p.change.sample(p.now);
            check(p.arms==1 && pad.owns,"lateral controls remain bounded and held/repeated polls cannot replay");
            check(pad.x==(elapsed<50?0:32767) && pad.y==(elapsed<50?0:-8192),
                "lateral recipe owns neutral then the measured native axes at every cadence");
            check(!p.change.evidence().physics_changed,"neither input delivery nor raw identity manufactures completion");
        }
        auto wrong=p.contact;wrong.identity=59;
        p.change.verify(p.now,{true,403,402,1,{}},wrong,p.animation);
        check(!p.change.evidence().physics_changed,"transient native identity59 is not Bluntslide31");
        wrong=p.contact;wrong.segment_identity[0]++;
        p.change.verify(p.now,{true,403,402,1,{}},wrong,p.animation);
        check(!p.change.evidence().physics_changed,"lateral completion on a different segment is not accepted");
        auto snapshot=p.animation;snapshot.grind_type=24;
        p.change.verify(p.now,{true,403,402,1,{}},p.contact,snapshot);
        check(p.change.evidence().physics_changed && !p.change.evidence().animation_changed,
            "lateral physics identity requires separate changed animation evidence");
        p.change.verify(p.now,{true,403,402,1,{}},p.contact,p.animation);
        check(p.change.evidence().animation_changed && !p.change.active(),"matching native Bluntslide animation retires its bounded lease");
    }
    for(unsigned fault=0;fault<9;++fault) {
        Pipeline p;p.contact.identity=24;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();p.step(1200,403);p.poll(0x6000);
        check(p.change.active(),"negative lateral case has a qualified source");
        p.step(1220,403);
        switch(fault) {
        case 0:p.poll(classic_a);break;
        case 1:p.poll(0,false);break;
        case 2:p.frame.owner.core++;p.poll();break;
        case 3:p.step(1220,701);p.poll();break;
        case 4:p.step(1220,100);p.poll();break;
        case 5:p.contact.segment_identity[0]++;p.poll();break;
        case 6:p.contact.valid=false;p.poll();break;
        case 7:p.step(1220,404);p.poll();break;
        case 8:p.now=2700;p.poll();break;
        }
        check(!p.change.active() && !p.change.sample(p.now).owns,"lateral cancellation withdraws control on ownership/geometry/exit/timeout or wrong destination");
    }
    // Exercise the same native request guard used by the production feature
    // override. Neutral wait must never select an unintended 50-50 return.
    for(unsigned fault=0;fault<20;++fault) {
        std::array<float,3> raw{1.5040742f,-.4f,0};unsigned reads{},validations{},pointer_reads{};
        if(fault==1)raw[0]=-raw[0];
        if(fault==2)raw[0]=0;
        if(fault==3)raw[1]=0;
        if(fault==4)raw[2]=.01f;
        if(fault==5)raw[0]=std::numeric_limits<float>::quiet_NaN();
        if(fault==6)raw[1]=-.46f;
        if(fault==7)raw[0]=2.5f;
        if(fault==16)raw[0]=55*std::numbers::pi_v<float>/180;
        if(fault==17)raw[0]=125*std::numbers::pi_v<float>/180;
        if(fault==18)raw[1]=-.45f;
        if(fault==19)raw[1]=-.1745329201f;
        const auto reader=[&](std::uintptr_t address,auto& value) {
            if constexpr(std::is_same_v<std::remove_reference_t<decltype(value)>,std::uintptr_t>) {
                if(address!=100+5248)return false;
                value=0x20000;++pointer_reads;
                if(fault==12)value=0;
                if(fault==13 && pointer_reads>1)value+=8;
                if(fault==14)value+=1;
                return fault!=15;
            } else {
                ++reads;unsigned index{};
                if(address==0x20000+24)index=0;
                else if(address==0x20000+60)index=1;
                else if(address==0x20000+52)index=2;
                else return false;
                value=raw[index];
                if(fault==8 && reads>3)value+=.01f;
                return fault!=9;
            }
        };
        const bool admitted=grind_control_admitted(GrindAdmissionControl::lateral_tail,100,fault==11?49:50,
            reader,[&]{return ++validations==1 || fault!=10;});
        check(admitted==(fault<2),"native guard accepts either wrapped lateral sign and rejects neutral, pitch/roll, stale/torn, missing or early evidence");
    }
    check(grind_control_admitted(GrindAdmissionControl::ordinary,0,0,
        [](auto,auto&){check(false,"ordinary admission must not read new lateral fields");return false;},[]{return true;}),
        "existing native admission is unchanged");
    std::cout<<checks<<" grind change production composition checks passed\n";
}
