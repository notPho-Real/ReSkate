#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
// Explicit provider fixtures test typed transport. They are not native enum or
// control-set evidence for Grind/Slide/Lip, whose actual mappings remain unknown.
struct Bridge {
    ArcadeInput input{true,InputPolicy::strict()};NativeInputState observer;
    NativeInputOwner owner{1,2,3,4,5};NativeRequestSample sample{};NativeInputFrame frame{};NativeFlipSelection flip{true,-1};
    Bridge(){input.enable(true);sample.valid=true;sample.state=401;sample.crouch_mode=1;}
    void observe(std::uint64_t now){frame=observer.observe(now,owner,sample,flip);input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);}
    NativeGesture::Output poll(std::uint64_t now,std::uint16_t buttons=0,short x=0,short y=0){observe(now);return input.poll(0,now,buttons,x,y,111,-222,true,false,false,0,false,false,false,now/17,false,{},false,false,SkatePose::riding);}
    bool contains(InputToken token)const{for(const auto& event:input.history(0)->events())if(event.token==token)return true;return false;}
};
}
int main(){
    for(const auto kind:{InputBalanceKind::manual,InputBalanceKind::nose,InputBalanceKind::grind,InputBalanceKind::slide,InputBalanceKind::lip}){
        Bridge b;b.observe(1000);
        const InputBalanceControls controls{kind,{InputSource::direction,8,InputEdge::press},{InputSource::direction,2,InputEdge::press},true};
        b.input.publish_balance_controls(controls,b.frame);b.poll(1000);b.poll(1017,0,0,25000);
        check(!b.contains(controls.first),"native-owned typed control publication reaches the shared journal exclusion consumer");
        b.poll(1034,0,0,0);b.poll(1051,0x2000);
        check(b.contains({InputSource::buttons,13,InputEdge::press}),"unrelated physical face input remains recorded during admitted balance controls");
        // A contact generation change invalidates the published binding. The
        // input engine deliberately samples using the preceding update's set.
        b.sample.words[1]=0x4000;b.poll(1068);b.poll(1085,0,0,25000);
        check(b.contains(controls.first),"changed native contact generation retires the old control set after the preceding-update boundary");
        b.input.publish_balance_controls(controls,b.frame);b.poll(1102);b.poll(1119,0,0,-25000);
        check(!b.contains(controls.second),"a newly admitted set binds to the new contact episode");
        b.input.enable(false);const auto off=b.poll(1136,0x4000,0,25000);
        check(off.buttons==0x4000&&off.right_x==111&&off.right_y==-222&&!b.input.busy(0),"Arcade OFF releases every owner and preserves physical controls");
    }
    for(unsigned fault=0;fault<4;++fault){
        Bridge b;b.observe(2000);const auto old=b.frame;
        b.input.publish_balance_controls(InputBalanceControls::vertical(InputBalanceKind::grind),old);b.poll(2000);
        if(fault==0){b.owner.rig=6;} // New full native owner.
        if(fault==1){b.sample.state=402;} // Action/contact context changed.
        if(fault==2){b.observer.invalidate();} // Menu/gap/reset episode.
        if(fault==3){b.flip.id=2;} // Native action selection changed without contact change.
        b.poll(2017);b.poll(2034,0,0,25000);
        check(b.contains({InputSource::direction,8,InputEdge::press}),"old owner/action/reset-generation controls cannot suppress new physical events");
    }
    for(bool nose:{false,true}){
        Bridge b;b.sample.state=100;b.sample.words[1]=nose?0x8000:0x4000;b.observe(2500);
        ManualObservation identity;const auto pose=nose?SkatePose::nose_manual:SkatePose::manual;
        ManualObservationContext context{2500,b.frame.owner_generation,true,true,true,false,false,pose};
        identity.poll(SkatePose::riding,{true,0,0,0,0,ScoreLabelUncertainty::none},context);
        b.observe(2517);context.now=2517;
        const auto admitted=identity.poll(pose,{true,score_label_bit(nose?2u:1u),1,2517,2517,ScoreLabelUncertainty::none},context);
        check(admitted.admitted&&admitted.confirmed,"independent native identity plus contact admits the Manual/Nose control publisher");
        b.input.publish_balance_controls(InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual),b.frame);
        b.poll(2517);b.poll(2534,0,0,25000);
        check(!b.contains({InputSource::direction,8,InputEdge::press}),"identity/contact through typed facade preserves native balance correction exclusions");
        b.input.invalidate_authority();b.poll(2551);b.poll(2568,0,0,-25000);
        check(b.contains({InputSource::direction,2,InputEdge::press}),"authority loss clears old typed admission even before owner addresses change");
    }
    Bridge negative;negative.observe(3000);auto controls=InputBalanceControls::vertical(InputBalanceKind::lip);controls.admitted=false;
    negative.input.publish_balance_controls(controls,negative.frame);negative.poll(3000);negative.poll(3017,0,0,25000);
    check(negative.contains(controls.first),"unadmitted typed metadata never excludes a physical control");
    std::cout<<checks<<" native balance exclusion bridge checks passed\n";
}
