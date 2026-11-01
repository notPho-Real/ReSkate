#include "Extension/ArcadeMode/arcade_grind_change.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include <cstdlib>
#include <iostream>
#include <cstring>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
constexpr std::uint16_t test_b=0x2000,test_y=0x8000;
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
    NativeGrindControl controls{true,true};
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
        change.observe(now,frame,contact,allowed && !(buttons&classic_a),animation,controls);
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
    void advance(bool native_idle=true,bool arm_allowed=true) {
        change.advance(now,contact,native_idle,[&](auto,auto deadline,auto admission_control) noexcept {
            ++arms;last_deadline=deadline;control=admission_control;return arm_allowed;
        });
    }
};
}
int main() {
    for(unsigned hz:{30u,60u,120u})for(auto target:{GrindChangeTarget::noseslide,GrindChangeTarget::nosebluntslide,
            GrindChangeTarget::bluntslide,GrindChangeTarget::tailslide})for(bool together:{false,true}) {
        const bool nose=target==GrindChangeTarget::noseslide || target==GrindChangeTarget::nosebluntslide;
        const bool reverse=target==GrindChangeTarget::nosebluntslide || target==GrindChangeTarget::tailslide;
        if(reverse && together)continue; // Existing simultaneous priorities stay unchanged.
        const unsigned preparation=nose?23u:24u;
        const unsigned identity=nose?(reverse?30u:21u):(reverse?22u:31u);
        const std::uint16_t first=target==GrindChangeTarget::noseslide?test_y:
            target==GrindChangeTarget::tailslide?classic_x:test_b;
        const std::uint16_t held=target==GrindChangeTarget::nosebluntslide?test_b|test_y:
            target==GrindChangeTarget::bluntslide?test_b|classic_x:classic_x|test_y;
        Pipeline p;p.contact.end[2]=-12;p.establish();p.step(1200);p.poll(together?held:first);
        if(!together){p.step(1230);p.poll(held);}
        const auto start=p.now,initial_serial=p.change.serial();
        check(p.arms==1 && p.change.active() && p.dispatched.grind_target()==target,
            "requested slide from native50-50 arms its qualified preparation, not another recognizer");
        check(p.control==GrindAdmissionControl::ordinary,"preparation retains original401 admission");
        for(unsigned i=0;i<700*hz/1000 && p.arms==1;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.step(start+elapsed,elapsed<150?401:elapsed<400?406:403);
            if(elapsed>=400){p.contact.actor=32;p.contact.identity=preparation;}
            p.poll(held);p.poll(held);
            p.change.verify(p.now,elapsed>=400?NativeGrindCompletion{true,401,403,1,{}}:NativeGrindCompletion{},p.contact,p.animation);
            check(!p.change.evidence().completed && !p.change.evidence().physics_changed && !p.change.evidence().animation_changed,
                "preparation completion, physics and animation never acknowledge the requested slide");
            if(elapsed>=400 && elapsed<500) {
                const auto pad=p.change.sample(p.now);
                check(pad.owns && pad.x==0 && pad.y==0,"verified preparation withdraws into owned neutral while original lease retires");
            }
            p.advance(elapsed>=500);p.advance(elapsed>=500);
        }
        const auto final_start=p.change.evidence().final_leg_started_at;
        check(p.arms==2 && p.change.serial()==initial_serial+1 && final_start>=start+500,
            "retired original lease and verified native intermediate admit one fresh lateral lease");
        check(p.last_deadline==final_start+1500 && p.change.evidence().preparation_completed &&
            p.change.evidence().preparation_physics_changed,"each native leg retains its own bounded lifetime and evidence");
        check(p.control==(nose?(reverse?GrindAdmissionControl::lateral_nose_reverse:GrindAdmissionControl::lateral_nose):
            (reverse?GrindAdmissionControl::lateral_tail_reverse:GrindAdmissionControl::lateral_tail)),
            "second admission waits for the correct raw native lateral band");
        for(unsigned i=0;i<=1600*hz/1000;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.step(final_start+elapsed,elapsed<150?403:elapsed<700?406:402);
            if(elapsed>=700){p.contact.actor=40;p.contact.identity=identity;}
            p.animation.grind_type=elapsed>=900?identity:preparation;
            p.animation.physics_grind_type=p.contact.identity;
            p.poll(held);p.poll(held);
            p.change.verify(p.now,elapsed<700?NativeGrindCompletion{true,401,403,1,{}}:
                NativeGrindCompletion{true,403,402,1,{}},p.contact,p.animation);
            p.advance();p.advance();
            check(p.arms==2,"held physical pair, repeated polls and old completion cannot arm a third native leg");
            const auto& evidence=p.change.evidence();
            check(evidence.first_input_at==1200 && evidence.last_input_at==start && evidence.started_at==start &&
                evidence.serial==initial_serial && evidence.final_leg_serial==initial_serial+1,
                "continuation retains original physical timestamps and distinct native lease serials");
            if(elapsed<700)check(!evidence.completed && !evidence.physics_changed,"first completion cannot satisfy second leg");
            if(elapsed>=700 && elapsed<900)check(evidence.physics_changed && !evidence.animation_changed,
                "final native identity still requires independent changed animation");
            const auto pad=p.change.sample(p.now);
            const bool retained=elapsed<(reverse && !nose?1500u:1000u);
            check(pad.owns==retained,"final trajectory duration uses its own start, not the old physical event age");
            if(pad.owns) {
                const bool neutral=elapsed<50 || (reverse && !nose && elapsed>=1050);
                check(pad.x==(neutral?0:reverse?-32767:32767) && pad.y==(neutral?0:nose?8192:-8192),
                    "two-leg routes reproduce neutral/lateral/withdrawal controls at each cadence");
            }
        }
        check(p.change.evidence().completed && p.change.evidence().physics_changed && p.change.evidence().animation_changed,
            "only independent second completion, final physics and animation satisfy the slide request");
    }
    for(unsigned hz:{30u,60u,120u}) {
        Pipeline p;p.contact.end[2]=-12;p.establish();p.step(1200);p.poll(classic_x);
        p.step(1210);p.poll();p.step(1230);p.poll(classic_x);
        const auto initial_serial=p.change.serial();
        check(p.arms==1 && p.change.active() && p.dispatched.grind_target()==GrindChangeTarget::smith,
            "X then X from50-50 arms only the native5-0 preparation");
        for(unsigned i=0;i<650*hz/1000 && !p.change.evidence().final_leg_serial;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.step(1230+elapsed,elapsed<150?401:elapsed<400?406:403);
            if(elapsed>=400){p.contact.actor=32;p.contact.identity=24;}
            p.controls.neutral=elapsed>=500;
            p.poll(classic_x);p.poll(classic_x);
            p.change.verify(p.now,elapsed>=400?NativeGrindCompletion{true,401,403,1,{}}:NativeGrindCompletion{},p.contact,p.animation);
            p.advance(elapsed>=450);p.advance(elapsed>=450);
            if(elapsed>=400 && elapsed<500) {
                const auto pad=p.change.sample(p.now);
                check(pad.owns && pad.x==0 && pad.y==0 && !p.change.evidence().control_only,
                    "Smith waits in owned neutral for native lease retirement and observed control neutral");
            }
        }
        const auto start=p.change.evidence().final_leg_started_at;
        check(start>=1730 && p.change.serial()==initial_serial+1 && p.arms==1 && !p.change.requires_transition(),
            "verified preparation hands off to original403 control motion without arming a second406");
        for(unsigned i=0;i<=2300*hz/1000;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.step(start+elapsed,403);p.contact.identity=elapsed<800?24u:elapsed<1200?71u:26u;
            p.animation.grind_type=elapsed<1400?24u:26u;p.animation.physics_grind_type=p.contact.identity;
            p.poll(classic_x);p.poll(classic_x);
            p.change.verify(p.now,{true,401,403,1,{}},p.contact,p.animation);p.advance();
            const auto& evidence=p.change.evidence();
            check(p.arms==1 && !evidence.completed && evidence.control_only && evidence.preparation_completed,
                "preparation completion remains distinct from Smith's control-only execution");
            check(evidence.first_input_at==1200 && evidence.last_input_at==1230 && evidence.started_at==1230,
                "Smith continuation retains physical press times without a re-dated request");
            if(elapsed<1200)check(!evidence.physics_changed,"prior preparation age and Suski cannot qualify Smith early");
            if(elapsed>=1200 && elapsed<1400)check(evidence.physics_changed && !evidence.animation_changed,
                "actual Smith physics after its own ramp still needs matching changed animation");
            const auto pad=p.change.sample(p.now);
            check(pad.owns==(elapsed<2200),"Smith continuation retains its own bounded ramp and neutral withdrawal");
            if(pad.owns) {
                const std::int16_t x=elapsed>=200 && elapsed<500?24000:0;
                constexpr std::array<std::int16_t,5> ramp{6553,13106,19660,26213,32767};
                const auto step=elapsed>=800?(elapsed-800)/100:0;
                const std::int16_t y=elapsed<500?-32767:elapsed<800 || elapsed>=2000?0:ramp[step<4?step:4];
                check(pad.x==x && pad.y==y,"Smith starts captured-reference controls after, not during, preparation");
            }
        }
        check(p.change.evidence().physics_changed && p.change.evidence().animation_changed && !p.change.active(),
            "the independent final Smith identity and animation remain recorded after control withdrawal");
    }
    for(unsigned fault=0;fault<3;++fault) {
        Pipeline p;p.contact.end[2]=-12;p.establish();p.step(1200);p.poll(classic_x);
        p.step(1210);p.poll();p.step(1230);p.poll(classic_x);
        p.step(1330,406);p.poll();p.step(1430,403);p.contact.actor=32;p.contact.identity=24;
        p.controls={fault!=0,fault==2};p.poll();p.change.verify(p.now,{true,401,403,1,{}},p.contact,p.animation);
        p.advance(fault!=2);
        check(p.change.active() && !p.change.evidence().control_only && p.arms==1,
            "missing or non-neutral native controls and pending lease keep Smith handoff disabled");
        p.step(1530,403);p.poll();p.advance(fault!=2);
        p.step(1581,403);p.poll();p.advance(fault!=2);
        check(!p.change.active() && !p.change.sample(p.now).owns && p.arms==1,
            "unverified Smith handoff expires without stale controls or another native lease");
    }
    for(unsigned fault=0;fault<12;++fault) {
        Pipeline p;p.contact.end[2]=-12;p.establish();p.step(1200);p.poll(test_b);p.step(1230);p.poll(test_b|classic_x);
        p.step(1330,406);p.poll();p.step(1430,403);p.contact.actor=32;p.contact.identity=24;p.poll();
        auto done=NativeGrindCompletion{true,401,403,1,{}};
        if(fault==0)done.valid=false;
        if(fault==1)done.phase=.5f;
        if(fault==2)p.contact.identity=23;
        if(fault==3)p.contact.segment_identity[0]++;
        p.change.verify(p.now,done,p.contact,p.animation);
        if(fault==4)p.contact.actor++;
        if(fault==5)p.contact.valid=false;
        if(fault==6){p.now+=151;}
        if(fault==7){p.step(1440,403);p.frame.owner.rig++;p.poll();}
        if(fault==8){p.step(1440,403);p.poll(classic_a);}
        if(fault==9){p.step(1440,403);p.poll(0,false);}
        p.advance(fault!=10,fault!=11);
        check(p.arms==(fault==11?2u:1u),"missing preparation, stale/contact/actor/owner/input loss or live old lease cannot begin a second leg");
        check(!p.change.evidence().completed && !p.change.evidence().physics_changed && !p.change.evidence().animation_changed,
            "failed continuation never acknowledges or fabricates the requested name");
        if(fault==11)check(!p.change.active() && !p.change.sample(p.now).owns,"rejected native continuation withdraws input immediately");
    }
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
        if(fault==1)p.contact.identity=25;
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
    for(unsigned hz:{30u,60u,120u})for(bool together:{false,true})for(unsigned recipe:{0u,1u,2u,3u}) {
        const bool nose=recipe==1 || recipe==2,reverse=recipe>=2,tail_release=recipe==3;
        // Simultaneous B+Y/Y+X retain Nosegrind/Noseslide priority respectively.
        if(reverse && together)continue;
        const auto source_identity=nose?23u:24u,target_identity=tail_release?22u:reverse?30u:nose?21u:31u,intermediate=tail_release?71u:reverse?26u:nose?66u:59u;
        const std::uint16_t held=tail_release?0xc000u:reverse?0xa000u:nose?0xc000u:0x6000u;
        Pipeline p;p.contact.identity=source_identity;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();
        p.step(1200,403);p.poll(together?held:tail_release?0x4000:reverse?0x2000:nose?0x8000:0x2000);
        if(!together){p.step(1230,403);p.poll(held);}
        const auto start=p.now;
        check(p.change.active() && p.arms==1 && p.control==(tail_release?GrindAdmissionControl::lateral_tail_reverse:reverse?GrindAdmissionControl::lateral_nose_reverse:nose?GrindAdmissionControl::lateral_nose:GrindAdmissionControl::lateral_tail),
            "sequential and simultaneous Y-X/B-X dispatch the measured lateral lease only from its qualified source");
        check(p.dispatched.grind_target()==(tail_release?GrindChangeTarget::tailslide:reverse?GrindChangeTarget::nosebluntslide:nose?GrindChangeTarget::noseslide:GrindChangeTarget::bluntslide) && p.dispatched.first_input_at==1200 &&
            p.dispatched.last_input_at==start,"lateral dispatch preserves physical event timestamps");
        for(unsigned i=0;i<=1100*hz/1000;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.contact.identity=elapsed<700?source_identity:elapsed<800?intermediate:target_identity;
            p.step(start+elapsed,elapsed<100?403:elapsed<700?406:402);p.poll(held);p.poll(held);
            const auto pad=p.change.sample(p.now);
            check(p.arms==1 && pad.owns,"lateral controls remain bounded and held/repeated polls cannot replay");
            const bool neutral=elapsed<50 || (tail_release && elapsed>=1050);
            check(pad.x==(neutral?0:reverse?-32767:32767) && pad.y==(neutral?0:nose?8192:-8192),
                "lateral recipe owns neutral then the measured native axes at every cadence");
            check(!p.change.evidence().physics_changed,"neither input delivery nor raw identity manufactures completion");
        }
        auto wrong=p.contact;wrong.identity=intermediate;
        p.change.verify(p.now,{true,403,402,1,{}},wrong,p.animation);
        check(!p.change.evidence().physics_changed,"transient native identity59/66/26/71 cannot satisfy the requested destination");
        wrong.identity=reverse?21u:nose?30u:21u;p.change.verify(p.now,{true,403,402,1,{}},wrong,p.animation);
        check(!p.change.evidence().physics_changed,"Nosebluntslide or another native slide cannot substitute for the measured name");
        wrong=p.contact;wrong.segment_identity[0]++;
        p.change.verify(p.now,{true,403,402,1,{}},wrong,p.animation);
        check(!p.change.evidence().physics_changed,"lateral completion on a different segment is not accepted");
        auto snapshot=p.animation;snapshot.grind_type=source_identity;
        p.change.verify(p.now,{true,403,402,1,{}},p.contact,snapshot);
        check(p.change.evidence().physics_changed && !p.change.evidence().animation_changed,
            "lateral physics identity requires separate changed animation evidence");
        p.change.verify(p.now,{true,403,402,1,{}},p.contact,p.animation);
        check(p.change.evidence().animation_changed && p.change.active()==tail_release,"independent animation retires the request only after its measured control trajectory");
        if(tail_release) {
            const auto pad=p.change.sample(p.now);
            check(pad.owns && pad.x==0 && pad.y==0,"Tailslide owns the measured neutral tail after animation acknowledgment");
            p.step(start+1500,402);p.poll(held);
            check(!p.change.active() && !p.change.sample(p.now).owns && p.arms==1,"Tailslide releases at deadline without another action from held input");
        }
    }
    for(unsigned fault=0;fault<9;++fault)for(unsigned recipe:{0u,1u,2u,3u}) {
        const bool nose=recipe==1 || recipe==2,reverse=recipe>=2,tail_release=recipe==3;
        Pipeline p;p.contact.identity=nose?23u:24u;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();
        if(reverse){p.step(1170,403);p.poll(tail_release?0x4000:0x2000);}
        p.step(1200,403);p.poll(tail_release?0xc000:reverse?0xa000:nose?0xc000:0x6000);
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
    for(unsigned hz:{30u,60u,120u}) {
        Pipeline p;p.contact.identity=31;p.contact.end[2]=-12;
        p.step(1000,402);p.poll();p.step(1100,402);p.poll();
        check(p.change.admission().ready && p.change.admission().excluded_variant==
            grind_change_variant(GrindChangeTarget::bluntslide),"actual Bluntslide excludes unchanged Bluntslide");
        p.step(1200,402);p.poll(classic_x);p.step(1210,402);p.poll();p.step(1230,402);p.poll(test_b);
        check(p.arms==1 && p.change.active() && p.control==GrindAdmissionControl::slide_tail_return &&
            p.dispatched.grind_target()==GrindChangeTarget::grind_5_0 && p.dispatched.first_input_at==1200 &&
            p.dispatched.last_input_at==1230,"ordered X then B admits measured402 to5-0 with original timestamps");
        for(unsigned i=0;i<=1490*hz/1000;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            if(elapsed>=810)p.contact.identity=24;
            p.step(1230+elapsed,elapsed<250?402:elapsed<780?406:403);
            p.poll(test_b);p.poll(test_b);
            const auto pad=p.change.sample(p.now);
            check(pad.owns && pad.x==(elapsed<200 || elapsed>=800?0:32767) &&
                pad.y==(elapsed<200?-32767:elapsed<800?-8192:0),
                "402 return preserves Down,600ms sweep and neutral independently of poll count");
            check(p.arms==1,"held B cannot replay a return lease");
            if(elapsed>=810) {
                if(!p.change.evidence().completed) {
                    p.change.verify(p.now,{true,403,402,1,{}},p.contact,p.animation);
                    check(!p.change.evidence().completed,"previous Bluntslide completion is not402 return completion");
                    auto wrong=p.contact;wrong.segment_identity[0]++;
                    p.change.verify(p.now,{true,402,403,1,{}},wrong,p.animation);
                    check(!p.change.evidence().physics_changed,"402 return requires independently retained segment");
                    auto old=p.animation;old.grind_type=31;
                    p.change.verify(p.now,{true,402,403,1,{}},p.contact,old);
                    check(p.change.evidence().physics_changed && !p.change.evidence().animation_changed,
                        "5-0 physics alone cannot acknowledge unchanged animation");
                }
                p.change.verify(p.now,{true,402,403,1,{}},p.contact,p.animation);
                check(p.change.evidence().animation_changed && p.change.active(),
                    "early native acknowledgment cannot skip bounded neutral withdrawal");
            }
        }
        p.step(2730,403);p.poll(test_b);
        check(!p.change.active() && !p.change.sample(p.now).owns,"402 return expires without synthetic control leakage");
        p.step(2740,403);p.poll(test_b);p.step(2840,403);p.poll(test_b);
        check(p.arms==1 && p.change.admission().ready,"actual returned5-0 establishes for another physical pair");
        p.step(2900,403);p.poll(test_y);p.step(2910,403);p.poll();p.step(2930,403);p.poll(test_y);
        check(p.arms==2 && p.control==GrindAdmissionControl::ordinary &&
            p.dispatched.grind_target()==GrindChangeTarget::grind_50_50,
            "new Y then Y after402 return enters the existing native5-0 to50-50 executor");
    }
    for(bool simultaneous:{false,true}) {
        Pipeline p;p.contact.identity=31;p.contact.end[2]=-12;
        p.step(1000,402);p.poll();p.step(1100,402);p.poll();
        if(simultaneous) {p.step(1200,402);p.poll(classic_x|test_b);}
        else {p.step(1200,402);p.poll(test_y);p.step(1210,402);p.poll();p.step(1230,402);p.poll(test_y);}
        check(p.arms==0 && !p.change.active(),
            "simultaneous BX remains unchanged Bluntslide and402 neutral is not an unqualified50-50 alias");
    }
    for(unsigned fault=0;fault<7;++fault) {
        Pipeline p;p.contact.identity=31;p.contact.end[2]=-12;
        if(fault==0)p.contact.end[2]=-6;
        if(fault==1)p.contact.identity=21;
        if(fault==2)p.contact.identity=22;
        if(fault==3)p.contact.identity=30;
        if(fault==4)p.contact.geometry=2;
        if(fault==5)p.contact.begin[1]=1;
        p.step(1000,fault==6?404:402);p.poll();p.step(1100,fault==6?404:402);p.poll();
        check(!p.change.admission().ready,"only measured402/31 horizontal12m source admits slide return");
    }
    for(unsigned fault=0;fault<8;++fault) {
        Pipeline p;p.contact.identity=31;p.contact.end[2]=-12;
        p.step(1000,402);p.poll();p.step(1100,402);p.poll();p.step(1200,402);p.poll(classic_x);
        p.step(1210,402);p.poll();p.step(1230,402);p.poll(test_b);
        check(p.change.active(),"negative402 return case armed");
        p.step(1300,402);
        if(fault==0)p.contact.actor++;
        if(fault==1)p.contact.segment_identity[0]++;
        if(fault==2)p.contact.valid=false;
        if(fault==3) {p.owner.entity++;p.step(1300,402);}
        if(fault==4)p.step(1300,701);
        if(fault==5)p.now+=151;
        p.poll(fault==6?classic_a:0,fault!=7);
        check(!p.change.active() && !p.change.sample(p.now).owns,
            "slide return cancels on actor,segment,contact,owner,exit,stale,A or OFF loss");
    }
    for(unsigned fault=0;fault<24;++fault) {
        std::array<float,3> raw{.19176197f,-.3494857f,0};
        unsigned reads{},pointers{},validations{},active_reads{},published_reads{};
        if(fault==1)raw[0]=0;
        if(fault==2)raw[0]=-raw[0];
        if(fault==3)raw[0]=35*std::numbers::pi_v<float>/180;
        if(fault==4)raw[1]=-.5f;
        if(fault==5)raw[1]=-.1745329201f;
        if(fault==6)raw[2]=.001f;
        if(fault==7)raw[0]=std::numeric_limits<float>::quiet_NaN();
        if(fault==8)raw[1]=.4f;
        if(fault==23)raw[0]+=2*std::numbers::pi_v<float>;
        const auto read=[&](std::uintptr_t address,auto& value) {
            using T=std::remove_reference_t<decltype(value)>;
            if constexpr(std::is_same_v<T,std::uintptr_t>) {
                if(address!=100+5248)return false;
                ++pointers;value=(fault==12?0u:0x20000u)+(fault==13 && pointers>1?8u:0u);
                return fault!=14;
            } else if constexpr(std::is_same_v<T,std::uint8_t>) {
                value=1;
                if(address==0x30000+766) {++active_reads;if(fault==15 || (fault==17 && active_reads>1))value=0;}
                else if(address==0x40000+419) {++published_reads;if(fault==16 || (fault==18 && published_reads>1))value=0;}
                else return false;
                return fault!=19;
            } else {
                ++reads;
                if(address==0x20000+24)value=raw[0];
                else if(address==0x20000+60)value=raw[1];
                else if(address==0x20000+52)value=raw[2];
                else return false;
                if(fault==10 && reads>3)value+=.01f;
                return fault!=11;
            }
        };
        const bool admitted=grind_control_admitted(GrindAdmissionControl::slide_tail_return,100,
            fault==9?199:200,read,[&]{return ++validations==1 || fault!=20;},
            fault==21?0:0x30000,fault==22?0:0x40000);
        check(admitted==(fault==0 || fault==23),
            "slide lease requires actual bounded active controls, original native admission and stable owner reads");
    }
    for(unsigned state:{401u,402u,403u,404u})for(unsigned identity:{20u,21u,22u,23u,24u,30u,31u,32u}) {
        check(grind_admission_source(GrindAdmissionControl::slide_tail_return,state,identity)==(state==402 && identity==31),
            "native arm source policy does not generalize slide return to unmeasured identities");
        check(grind_admission_source(GrindAdmissionControl::ordinary,state,identity),"ordinary arm source policy unchanged");
        for(auto policy:{GrindAdmissionControl::lateral_tail,GrindAdmissionControl::lateral_tail_reverse,
                         GrindAdmissionControl::lateral_nose,GrindAdmissionControl::lateral_nose_reverse}) {
            const bool nose=policy==GrindAdmissionControl::lateral_nose || policy==GrindAdmissionControl::lateral_nose_reverse;
            check(grind_admission_source(policy,state,identity)==(state==403 && identity==(nose?23u:24u)),
                "prior lateral arm source scope remains unchanged");
        }
    }
    // Exercise the same native request guard used by the production feature
    // override. Neutral wait must never select an unintended 50-50 return.
    for(unsigned fault=0;fault<20;++fault)for(auto policy:{GrindAdmissionControl::lateral_tail,GrindAdmissionControl::lateral_nose,GrindAdmissionControl::lateral_nose_reverse,GrindAdmissionControl::lateral_tail_reverse}) {
        const bool nose=policy==GrindAdmissionControl::lateral_nose || policy==GrindAdmissionControl::lateral_nose_reverse;
        const bool reverse=policy==GrindAdmissionControl::lateral_nose_reverse || policy==GrindAdmissionControl::lateral_tail_reverse;
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
        if(nose)raw[1]=-raw[1];
        if(reverse)raw[0]=-raw[0];
        const auto reader=[&](std::uintptr_t address,auto& value) {
            if constexpr(std::is_same_v<std::remove_reference_t<decltype(value)>,std::uintptr_t>) {
                if(address!=100+5248)return false;
                value=0x20000;++pointer_reads;
                if(fault==12)value=0;
                if(fault==13 && pointer_reads>1)value+=8;
                if(fault==14)value+=1;
                return fault!=15;
            } else if constexpr(std::is_same_v<std::remove_reference_t<decltype(value)>,std::uint8_t>) {
                check(false,"existing lateral admission does not read slide admission bytes");return false;
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
        const bool admitted=grind_control_admitted(policy,100,fault==11?49:50,
            reader,[&]{return ++validations==1 || fault!=10;});
        check(admitted==(fault==0 || (fault==1 && !reverse)),"native guard enforces qualified heading/pitch polarity and rejects neutral, pitch/roll, stale/torn, missing or early evidence");
    }
    for(unsigned fault=0;fault<4;++fault) {
        Pipeline p;p.contact.identity=23;p.contact.end[2]=fault==0?-6.f:-12.f;
        if(fault==1)p.contact.identity=24;
        if(fault==2)p.contact.geometry=2;
        if(fault==3)p.contact.begin[1]=1;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();p.step(1200,403);p.poll(0xc000);
        check(p.arms==0,"Noseslide requires actual Nosegrind source and measured horizontal twelve-metre geometry");
    }
    for(unsigned fault=0;fault<4;++fault) {
        Pipeline p;p.contact.identity=fault==1?23u:24u;p.contact.end[2]=fault==0?-6.f:-12.f;
        if(fault==2)p.contact.geometry=2;
        if(fault==3)p.contact.begin[1]=1;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();p.step(1200,403);p.poll(0x4000);
        p.step(1230,403);p.poll(0xc000);
        check(p.arms==0,"Tailslide requires actual5-0 source and measured horizontal twelve-metre geometry");
    }
    {
        Pipeline p;p.contact.identity=24;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();p.step(1200,403);p.poll(0xc000);
        check(p.dispatched.grind_target()==GrindChangeTarget::noseslide && p.arms==0,
            "simultaneous X+Y retains Noseslide priority instead of inventing an ordered Tailslide alias");
    }
    {
        Pipeline p;p.contact.identity=24;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();p.step(1200,403);p.poll(0x4000);
        p.step(1230,403);p.poll(0xc000);const auto start=p.now;
        for(std::uint64_t elapsed=0;elapsed<1500;elapsed+=50) {
            p.contact.identity=elapsed<700?24u:22u;
            p.step(start+elapsed,elapsed<100?403:elapsed<700?406:402);p.poll(0xc000);
            if(elapsed>=700)p.change.verify(p.now,{true,403,402,1,{}},p.contact,p.animation);
            const auto pad=p.change.sample(p.now);
            check(p.change.active() && pad.owns,"early independent animation evidence cannot skip Tailslide control withdrawal");
            const bool neutral=elapsed<50 || elapsed>=1050;
            check(pad.x==(neutral?0:-32767) && pad.y==(neutral?0:-8192),"Tailslide switches to owned neutral at1050ms from request regardless of acknowledgment time");
        }
        p.step(start+1500,402);p.poll(0xc000);
        check(!p.change.active() && !p.change.sample(p.now).owns && p.change.evidence().animation_changed,
            "Tailslide expires cleanly while retaining only independently observed evidence");
    }
    {
        Pipeline p;p.contact.identity=23;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();
        p.step(1200,403);p.poll(0xa000);
        check(p.dispatched.grind_target()==GrindChangeTarget::nosegrind && p.arms==0,
            "simultaneous B+Y retains existing Nosegrind priority rather than inventing an ordered Nosebluntslide alias");
    }
    for(unsigned identity=69;identity<=76;++identity) {
        Pipeline p;p.contact.identity=identity;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();p.step(1200,403);p.poll(0xc000);
        check(!p.change.admission().ready && !p.change.active() && p.arms==0,
            "real Suski/Darklip contact alone cannot admit an unqualified named source");
    }
    for(unsigned hz:{30u,60u,120u}) {
        Pipeline p;p.contact.identity=24;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();
        p.step(1200,403);p.poll(classic_x);p.step(1210,403);p.poll();p.step(1230,403);p.poll(classic_x);
        check(p.change.active() && !p.change.requires_transition() && p.arms==0 &&
            p.dispatched.grind_target()==GrindChangeTarget::smith && p.dispatched.first_input_at==1200 &&
            p.dispatched.last_input_at==1230,"X then X starts the original control-only Smith path without arming406");
        const auto serial=p.change.serial();p.controls.neutral=false;
        for(unsigned i=0;i<2200*hz/1000;++i) {
            const auto elapsed=static_cast<std::uint64_t>(i)*1000/hz;
            p.contact.identity=elapsed<600?24u:elapsed<1400?71u:26u;
            p.step(1230+elapsed,403);p.poll(classic_x);p.poll(classic_x);
            const auto pad=p.change.sample(p.now);
            check(pad.owns && p.change.serial()==serial && p.arms==0,"held/repeated polls retain one bounded control request");
            if(elapsed<200)check(pad.x==0 && pad.y==-32767,"Down establishes original control reference");
            else if(elapsed<500)check(pad.x==24000 && pad.y==-32767,"heading sweep retains captured Down reference");
            else if(elapsed<800 || elapsed>=2000)check(pad.x==0 && pad.y==0,"neutral separates pitch gesture and withdraws after the measured pulse");
            else {
                constexpr std::array<std::int16_t,5> values{6553,13106,19660,26213,32767};
                const auto step=(elapsed-800)/100;
                check(pad.x==0 && pad.y==values[step<4?step:4],"slow Up ramp and800ms hold preserve measured physical timing");
            }
            if(elapsed>=1400) {
                auto wrong=p.contact;wrong.segment_identity[0]++;
                p.change.verify(p.now,{},wrong,p.animation);
                if(!p.change.evidence().physics_changed)check(!p.change.evidence().animation_changed,"another segment cannot acknowledge Smith");
                auto animation=p.animation;animation.grind_type=24;
                p.change.verify(p.now,{true,403,403,1,{}},p.contact,animation);
                check(!p.change.evidence().completed && p.change.evidence().control_only && p.change.evidence().physics_changed,
                    "real control-only physics is separate from a406 completion; bogus completion is ignored");
                p.change.verify(p.now,{},p.contact,p.animation);
                check(p.change.evidence().animation_changed && p.change.active(),"matching native animation cannot skip final neutral withdrawal");
            } else {
                p.change.verify(p.now,{},p.contact,p.animation);
                check(!p.change.evidence().physics_changed,"5-0 and intermediate Suski never acknowledge Smith");
            }
        }
        p.step(3430,403);p.poll(classic_x);
        check(!p.change.active() && !p.change.sample(p.now).owns && p.change.evidence().animation_changed,
            "Smith retires after bounded neutral with independent evidence retained");
        p.contact.identity=24;p.step(3530,403);p.poll(classic_x);p.step(3630,403);p.poll(classic_x);
        check(!p.change.active() && p.arms==0,"held X does not replay after another source is observed");
    }
    for(unsigned fault=0;fault<9;++fault) {
        Pipeline p;p.contact.identity=24;p.contact.end[2]=-12;
        p.step(1000,403);p.poll();p.step(1100,403);p.poll();
        p.step(1200,403);p.poll(classic_x);p.step(1210,403);p.poll();p.step(1230,403);p.poll(classic_x);
        check(p.change.active(),"Smith cancellation case starts");
        p.step(1300,403);
        if(fault==0)p.controls.valid=false;
        if(fault==1)p.contact.actor++;
        if(fault==2)p.contact.segment_identity[0]++;
        if(fault==3)p.contact.valid=false;
        if(fault==4)p.frame.owner_generation++;
        if(fault==5)p.step(1300,406);
        if(fault==6)p.now+=151;
        p.poll(fault==7?classic_a:0,fault!=8);
        check(!p.change.active() && !p.change.sample(p.now).owns && p.arms==0,
            "feature/owner/actor/contact/segment loss, unexpected406, stale input, A and OFF cancel Smith without a lease");
    }
    for(unsigned fault=0;fault<5;++fault) {
        Pipeline p;p.contact.identity=24;p.contact.end[2]=-12;
        if(fault==0)p.controls={};
        if(fault==1)p.controls.neutral=false;
        if(fault==2)p.contact.identity=23;
        if(fault==3)p.contact.end[2]=-6;
        p.step(1000,fault==4?401:403);p.poll();p.step(1100,fault==4?401:403);p.poll();
        p.step(1200,fault==4?401:403);p.poll(classic_x);p.step(1210,fault==4?401:403);p.poll();
        p.step(1230,fault==4?401:403);p.poll(classic_x);
        check(!p.change.active() && p.arms==0,"Smith requires neutral native controls, feature-off and the qualified source geometry");
    }
    for(unsigned state:{402u,403u})for(unsigned fault=0;fault<9;++fault) {
        std::array<std::uint8_t,0x15000> bytes{};
        const std::uintptr_t base=0x140000000,context=0x10008,flags=0x13000,inputs=0x14000;
        const auto put=[&](std::uintptr_t p,auto value){std::memcpy(bytes.data()+p,&value,sizeof(value));};
        put(context+5140,state);put(context+5280,flags);put(flags,base+0x6123098);
        put(context+5248,inputs);put(inputs+24,-.3117f);put(inputs+60,0.f);put(inputs+52,0.f);
        if(fault==1)put(flags+26,std::uint8_t{1});
        if(fault==2)put(inputs+60,.4f);
        if(fault==3)put(inputs+24,std::numeric_limits<float>::quiet_NaN());
        if(fault==4)put(flags,std::uintptr_t{0});
        unsigned valid_calls{},pitch_reads{},pointer_reads{};
        const auto control=read_grind_control(base,context,[&](auto p,auto& value){
            if(p==inputs+60 && ++pitch_reads==2 && fault==5)put(p,.1f);
            if(p==context+5248 && ++pointer_reads==2 && fault==6)put(p,inputs+8);
            if(fault==7 && p==flags+26)return false;
            if(p+sizeof(value)>bytes.size())return false;
            std::memcpy(&value,bytes.data()+p,sizeof(value));return true;
        },[&]{return ++valid_calls<2 || fault!=8;},state);
        check(control.valid==(fault==0 || fault==2) && control.neutral==(fault==0),
            "native control reader distinguishes neutral from held input and rejects missing, raced, invalid or replaced state");
        if(fault==0) {
            const auto legacy=read_grind_control(base,context,[&](auto p,auto& value) {
                if(p+sizeof(value)>bytes.size())return false;
                std::memcpy(&value,bytes.data()+p,sizeof(value));return true;
            },[]{return true;});
            check(legacy.valid==(state==403),"existing default control reader remains403-only");
        }
    }
    check(grind_control_admitted(GrindAdmissionControl::ordinary,0,0,
        [](auto,auto&){check(false,"ordinary admission must not read new lateral fields");return false;},[]{return true;}),
        "existing native admission is unchanged");
    std::cout<<checks<<" grind change production composition checks passed\n";
}
