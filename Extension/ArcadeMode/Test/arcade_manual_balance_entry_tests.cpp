#include "Extension/ArcadeMode/arcade_manual_balance_entry.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_manual_hop.h"
#include "Extension/ArcadeMode/arcade_manual_release.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
struct Native {
    NativeInputOwner owner{1,2,3,4,5};NativeInputState producer;NativeInputFrame frame{};
    NativeInputFrame step(std::uint64_t now,unsigned contacts=0xc000u,unsigned state=100,bool pushing=false){
        NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=1;
        sample.words[0]=pushing?0x40u:0u;sample.words[1]=contacts;
        frame=producer.observe(now,owner,sample,{true,-1});return frame;
    }
};
ManualInputDiagnostic command(bool nose,std::uint64_t at=1000){
    ManualInputDiagnostic result;result.owns=result.safe=true;result.request=nose?1:-1;result.request_at=at;return result;
}
struct Pipeline {
    bool nose{},admitted{},exiting{},failed{};float speed{};unsigned cadence;std::uint64_t now=1000;
    Native native;ArcadeInput input{true};GroundManualBalance ground;ManualObservation history;
    ManualAcquisition acquisition;ManualPhysics dynamics;BalanceControl controls;ComboSession combo;ManualRelease release;
    NativeGesture::Output output{};ManualObservationResult observation{};PhysicalBalanceSample modeled_balance{};std::uint64_t label_sequence{};
    explicit Pipeline(bool is_nose,unsigned interval,float board_speed=0):nose(is_nose),speed(board_speed),cadence(interval){input.enable(true);}
    unsigned wanted()const{return nose?0x8000u:0x4000u;}
    void step(unsigned contacts,short y=0,bool label=false,unsigned state=100,bool pushing=false){
        const auto frame=native.step(now,contacts,state,pushing);
        input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
        const auto physical=manual_acquisition_pose(frame);
        ScoreLabelOccurrence edge{true,0,label_sequence,0,now,ScoreLabelUncertainty::none};
        if(label){edge.labels=score_label_bit(nose?2:1);edge.sequence=++label_sequence;edge.observed_at=now;}
        if(release.pending())history.reset(edge);
        const auto named=history.poll(SkatePose::unknown,edge,
            {now,frame.owner_generation,true,true,false,false,false,physical,pushing});
        const auto owned=ground.sample(now,frame,true,
            !release.pending()&&!combo.manual_putdown()&&!combo.failed_section());
        observation=manual_balance_observation(owned,named);
        if(owned.confirmed)history.reset(edge);
        const bool cleanup=acquisition.step(now,frame,physical,observation.confirmed,true,pushing);
        const auto pose=cleanup&&balance_kind(observation.pose)==BalanceKind::manual?SkatePose::riding:
            observation.confirmed?observation.pose:physical==SkatePose::air?SkatePose::air:SkatePose::riding;
        const auto correction=controls.poll(now,balance_kind(pose),combo.model().state().balance,0,y,true);
        const auto modeled=manual_physics_pose(dynamics,pose,static_cast<float>(cadence)/1000,controls.manual_input(),acquisition.preserves_history(now,frame));
        modeled_balance=modeled;
        failed=combo.step(pose,static_cast<float>(cadence)/1000,correction,false,&modeled);
        const auto terminal=release.step(now,frame.owner_generation,true,physical,combo.manual_putdown()||cleanup);
        exiting=terminal.pending;
        admitted=observation.confirmed&&combo.model().state().balance_kind==BalanceKind::manual&&!exiting&&!combo.manual_putdown()&&!combo.failed_section();
        if(terminal.started||exiting||combo.manual_putdown()||combo.failed_section())input.cancel_manuals();
        if(combo.manual_putdown()||combo.failed_section())acquisition.reset();
        input.publish_balance_controls(admitted?InputBalanceControls::vertical(nose?InputBalanceKind::nose:InputBalanceKind::manual):InputBalanceControls{},frame);
        const auto control=manual_input_pose(pose,pushing,admitted,physical);
        output=input.poll(0,now,0,0,y,0,0,true,false,false,speed,false,false,false,now,false,{},false,true,control,
            false,false,false,false,true,false,0,0,ActionExperiment::classic,true,true,admitted);
        if(exiting&&!output.authored_playback&&!output.action.owned)output.right_x=output.right_y=0;
        acquisition.observe(now,frame,output.manual,true);ground.observe(now,frame,output.manual,true);
        now+=cadence;
    }
    void enter(){
        step(0xc000);step(0xc000,nose?-25000:25000);step(0xc000,nose?25000:-25000);
        check(output.manual.owns,"actual ground pair must first deliver the native Manual gesture");
        check(!admitted&&!balance_meter(combo.balance_display_state()).visible,"delivery without new physical contact cannot start a balance");
        step(wanted());
        check(admitted&&observation.admitted&&balance_meter(combo.balance_display_state()).visible,
            "both ground Manual families start physical balance before any score/history name");
    }
};
void composed(){
    for(bool nose:{false,true})for(unsigned cadence:{17u,34u})for(float speed:{0.0f,8.0f}){
        Pipeline p(nose,cadence,speed);p.enter();
        while(p.now<2500){
            const short correction=p.combo.model().state().balance_velocity>0?25000:-25000;
            const auto previous_time=p.modeled_balance.elapsed;
            p.step(p.wanted(),correction,p.now>=1500&&p.label_sequence==0);
            check(p.modeled_balance.elapsed>previous_time,"late name presentation cannot restart physical balance history");
            check(p.admitted&&!p.exiting&&!p.failed&&p.output.manual.owns,"corrected physical balance continues beyond the old unadmitted1200ms cleanup lease");
        }
    }
    for(bool nose:{false,true})for(bool unsafe:{false,true}){
        Pipeline p(nose,17);p.enter();
        const auto raw=static_cast<short>((nose!=unsafe)?-32768:32767);
        while(p.now<7000&&!p.combo.manual_putdown()&&!p.combo.failed_section())p.step(p.wanted(),raw);
        if(unsafe){
            check(p.combo.failed_section()&&p.failed&&!p.exiting,"unsafe physical endpoint routes failure without substituting safe putdown");
            check(balance_meter(p.combo.balance_display_state()).visible,"failed meter remains attached to the physically continuing native contact");
            for(unsigned i=0;i<5;++i){p.step(p.wanted(),0,i==2);check(p.combo.failed_section()&&!p.admitted&&!p.exiting,"late name and withdrawn output cannot restart a failed section");}
        }else{
            check(p.combo.manual_putdown()&&p.exiting&&!p.failed,"safe physical endpoint starts the existing native neutral putdown");
            check(p.output.right_y==0&&!balance_meter(p.combo.balance_display_state()).visible,"safe exit releases the native hold and hides active balance");
            for(unsigned i=0;i<5;++i){p.step(p.wanted(),0,i==2);check(p.exiting&&!p.admitted,"delayed name cannot reopen a section during putdown");}
        }
        p.step(0xc000);check(!p.admitted&&!p.exiting&&!p.combo.failed_section(),"actual two-wheel completion retires the terminal section");
        p.step(p.wanted());check(!p.admitted,"uncommanded later one-wheel contact cannot replay the spent entry");
        p.ground.reset();p.step(0xc000);p.step(0xc000,nose?-25000:25000);p.step(0xc000,nose?25000:-25000);p.step(p.wanted());
        check(p.admitted,"a genuinely new physical pair can start a fresh section after terminal completion");
    }
}
void guards(){
    for(bool nose:{false,true}){
        const unsigned wanted=nose?0x8000u:0x4000u;
        for(unsigned fault=0;fault<15;++fault){
            Native n;GroundManualBalance balance;auto delivered=command(nose);const auto baseline=n.step(1000);
            if(fault!=0)balance.observe(1000,baseline,delivered,true);
            auto contact=n.step(1017,wanted);
            if(fault==1)contact.owner.rig++;
            if(fault==2)contact.owner_generation++;
            if(fault==3)contact.valid=false;
            if(fault==4)contact.contact_generation=baseline.contact_generation;
            if(fault==5)contact.physics_step=baseline.physics_step;
            if(fault==6)contact.sampled_at=1018;
            if(fault==7)contact.contacts=nose?0x4000u:0x8000u;
            if(fault==8)contact.stance_and_push|=0x40u;
            if(fault==9)contact.state=403;
            if(fault==10)contact.mode=2;
            if(fault==11){delivered.owns=false;balance.observe(1010,baseline,delivered,true);}
            if(fault==12){balance.reset();}
            if(fault==13)contact.sampled_at=999;
            check(!balance.sample(1017,contact,fault!=14,true).confirmed,"missing/stale/reused/wrong-owner/opposite/pushing/withdrawn context cannot admit a balance");
        }
        for(unsigned initial:{wanted,nose?0x4000u:0x8000u,0u}){
            Native n;GroundManualBalance balance;balance.observe(1000,n.step(1000,initial),command(nose),true);
            check(!balance.sample(1017,n.step(1017,wanted),true,true).confirmed,"preexisting one-wheel or unknown contact cannot be relabeled a fresh delivered entry");
        }
        {
            Native n;GroundManualBalance balance;auto delivered=command(nose);
            balance.observe(1000,n.step(1000,0x4000u,100,true),delivered,true);
            check(!balance.sample(1017,n.step(1017,wanted),true,true).confirmed,"push recovery cannot stand in for a two-wheel delivery baseline");
            delivered.request_at=1034;balance.observe(1034,n.step(1034),delivered,true);
            check(balance.sample(1051,n.step(1051,wanted),true,true).admitted,"new post-push two-wheel delivery and matching contact admit either family");
        }
        for(unsigned delivery_age:{150u,151u}){
            Native n;GroundManualBalance balance;balance.observe(1000,n.step(1000),command(nose),true);
            n.step(1100);
            check(balance.sample(1000+delivery_age,n.step(1000+delivery_age,wanted),true,true).confirmed==(delivery_age<=150),"fresh physics cannot renew stale command delivery");
        }
        for(unsigned age:{1199u,1200u}){
            Native n;GroundManualBalance balance;const auto delivered=command(nose);balance.observe(1000,n.step(1000),delivered,true);
            for(std::uint64_t at=1100;at<1000+age;at+=100){const auto frame=n.step(at);balance.observe(at,frame,delivered,true);balance.sample(at,frame,true,true);}
            const auto result=balance.sample(1000+age,n.step(1000+age,wanted),true,true);
            check(result.confirmed==(age<1200),"repeated delivery cannot renew the original ground acquisition deadline");
        }
        for(unsigned gap:{99u,100u}){
            Native n;GroundManualBalance balance;balance.observe(1000,n.step(1000),command(nose),true);
            check(balance.sample(1017,n.step(1017,wanted),true,true).admitted,"real contact admits once");
            check(balance.sample(1017+gap,n.step(1017+gap),true,true).confirmed==(gap<100),"continuing basic balance keeps only the existing bounded two-wheel gap");
        }
        Native n;GroundManualBalance balance;auto delivered=command(nose);balance.observe(1000,n.step(1000),delivered,true);
        check(balance.sample(1017,n.step(1017,wanted),true,true).admitted,"original entry admits");
        balance.reset();balance.observe(1034,n.step(1034),delivered,true);
        check(!balance.sample(1051,n.step(1051,wanted),true,true).confirmed,"reset cannot rearm the same spent delivered command");
        delivered.request_at=1068;balance.observe(1068,n.step(1068),delivered,true);
        check(!balance.sample(1085,n.step(1085,wanted),true,false).confirmed,"closed lifecycle cannot admit a new command");
    }
}
}
int main(){composed();guards();std::cout<<checks<<" basic ground Manual balance entry checks passed\n";}
