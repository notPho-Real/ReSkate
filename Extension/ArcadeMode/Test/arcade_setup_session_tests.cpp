#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* why) {
    ++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}
}
struct Route {
    InputSession session{};
    NativeInputState producer{};
    NativeInputOwner owner{1,2,3,4,5};
    InputAdmission admission{};
    unsigned flips{},grabs{},contacts{0xc000};std::int32_t last_flip{};bool bailing{},switched{},omit_contact_proof{};
    std::uintptr_t selection_object{16};
    InputGenerations forged_idle_generations{};
    std::vector<PreparedGesture> catalog{
        {1,false,GestureContext::ground,{{0,0,-31000},{18,0,31000},{21,0,0},{24,0,0}}},
        {16,true,GestureContext::ground,{{0,0,31000},{18,0,-31000},{21,0,0},{24,0,0}}},
        {1,false,GestureContext::from_grind,{{0,0,-28000},{18,0,28000},{21,0,0},{24,0,0}}},
        {16,true,GestureContext::from_grind,{{0,0,28000},{18,0,-28000},{21,0,0},{24,0,0}}}};
    Route() {
        admission.gameplay=admission.authored=admission.may_start=admission.inspected_stance=true;
        admission.action_mode=ActionExperiment::classic;admission.catalog=catalog;
        admission.normal_flip={this,[](void* context,std::int32_t id) noexcept {
            auto& self=*static_cast<Route*>(context);
            check(self.admission.native_context.airborne,"flip callback requires genuine observed AIR");
            ++self.flips;self.last_flip=id;return true;
        }};
        admission.normal_grab.context=this;
        admission.normal_grab.submit=[](void* context,const InputRequest&,ArcadeTrick trick,bool held) noexcept {
            auto& self=*static_cast<Route*>(context);
            check(self.admission.native_context.airborne&&trick==ArcadeTrick::basic_bs_grab&&held,
                  "grab uses exact existing airborne profile and physical hold");
            ++self.grabs;return true;
        };
    }
    NativeGesture::Output poll(std::uint64_t now,unsigned state=100,int selected=-1,
                               std::uint16_t buttons=0,std::uint8_t lt=0,std::uint8_t rt=0) {
        NativeRequestSample raw{};raw.valid=true;raw.state=state;raw.crouch_mode=state==201?2u:1u;
        raw.words[1]=contacts;raw.words[0]=bailing?0x40u:switched?0x20u:0u;
        const auto frame=producer.observe(now,owner,raw,{true,selected,selection_object});
        admission.native_context=InputNativeContext::from_frame(frame);
        if(omit_contact_proof)admission.native_context.contacts_known=false;
        if(state==100&&forged_idle_generations.owner)admission.native_context.generations=forged_idle_generations;
        session.observe_manual_transition(admission.native_context);
        session.observe_rail_followup(admission.native_context);
        admission.pose=frame.airborne?SkatePose::air:state>=400&&state<=406?SkatePose::grind:SkatePose::riding;
        if(bailing)admission.pose=SkatePose::bail;
        admission.action_fresh=true;admission.flip_ready=frame.airborne&&selected==-1;
        admission.manual_safe=state==100;admission.tick=now*60/1000;
        return session.poll(now,{buttons,0,0,lt,rt},0,0,admission);
    }
};
void launch_selection() {
    for(unsigned cadence:{8u,16u,17u,33u,34u})for(bool nollie:{false,true}) {
        auto f=std::make_unique<Route>();f->poll(1000);
        const auto lt=f->poll(1000+cadence,100,-1,0,nollie?255:0);
        check(!lt.jump.setup_started&&!lt.authored_playback&&!lt.requested_id,"LT alone never launches or delivers a crouch");
        f->poll(1000+2*cadence);
        const auto at=1000+3*cadence;
        const auto setup=f->poll(at,100,-1,classic_a);
        check(setup.jump.setup_started&&setup.requested_id==(nollie?16u:1u)&&setup.jump.nollie==nollie,
              "tap LT then A selects exact native Nollie16; A alone selects Ollie1");
        check(setup.right_y==(nollie?31000:-31000),"exact prepared native crouch polarity is delivered");
        f->poll(at+cadence,100,nollie?16:1,classic_a);
        const auto release=f->poll(at+2*cadence,100,nollie?16:1);
        check(release.jump.released&&release.jump.setup_admitted&&release.jump.nollie==nollie&&release.jump.pressed_at==at,
              "physical A release retains admitted setup and original press time");
        f->poll(at+3*cadence,200,nollie?16:1);
        f->poll(at+4*cadence,201,nollie?16:1);
        f->poll(at+5*cadence,201,nollie?16:1,static_cast<std::uint16_t>(classic_x|4u));
        for(unsigned i=6;i<16;++i)f->poll(at+i*cadence,201,nollie?16:1);
        check(f->flips==1&&f->last_flip==(nollie?17:2),"released LT cannot relabel the flight's exact Kickflip identity");
    }
}
void held_retarget_and_second_tap() {
    for(bool initial_nollie:{false,true}) {
        auto f=std::make_unique<Route>();f->poll(1000);
        if(initial_nollie){f->poll(1010,100,-1,0,255);f->poll(1020);}
        const auto first=f->poll(1030,100,-1,classic_a);
        check(first.requested_id==(initial_nollie?16u:1u),"retarget starts exact selected setup");
        const auto changed=f->poll(1040,100,initial_nollie?16:1,classic_a,255);
        check(changed.authored_playback&&changed.requested_id==(initial_nollie?1u:16u)&&
              changed.right_y==(initial_nollie?-31000:31000),"LT during held A transactionally retargets exact native1 and16");
        const auto released=f->poll(1050,100,initial_nollie?1:16,0,255);
        check(released.jump.setup_admitted&&released.jump.pressed_at==1030&&released.jump.released_at==1050&&
              released.jump.nollie!=initial_nollie,"retarget preserves original A timing and prospective tap target");
        f->poll(1060,200,initial_nollie?1:16,0,255);
        f->poll(1070,201,initial_nollie?1:16);
        f->poll(1080,201,initial_nollie?1:16,static_cast<std::uint16_t>(classic_x|4u));
        for(unsigned i=0;i<10;++i)f->poll(1100+i*17,201,initial_nollie?1:16);
        if(f->flips!=1||f->last_flip!=(initial_nollie?2:17))std::fprintf(stderr,"retarget initial_nollie=%d flips=%u id=%d\n",initial_nollie,f->flips,f->last_flip);
        check(f->flips==1&&f->last_flip==(initial_nollie?2:17),"LT released after launch cannot toggle or relabel that flight again");
    }
}
void pressure_refuses_substitution() {
    for(bool a_before_hold:{false,true}) {
        auto f=std::make_unique<Route>();f->poll(1000);f->poll(1010,100,-1,a_before_hold?classic_a:0,255);
        for(unsigned at=1044;at<1210;at+=34)f->poll(at,100,-1,a_before_hold?classic_a:0,255);
        const auto pressure=f->poll(1210,100,-1,classic_a,255);
        check(!pressure.authored_playback&&!pressure.requested_id&&!pressure.jump.setup_started,
              "qualified Pressure has no unrelated ordinary or Nollie native fallback");
        const auto release=f->poll(1220,100,-1,0,255);
        check(!release.jump.setup_admitted&&!release.authored_playback,"unsupported Pressure cannot launch on A release");
        f->poll(1230,200,-1,static_cast<std::uint16_t>(classic_x|4u),255);
        check(!f->flips,"unsupported Pressure cannot submit an ordinary native flip");
    }
}
void pressure_release_resumes_same_a() {
    for(bool a_before_hold:{false,true})for(bool nollie:{false,true}) {
        auto f=std::make_unique<Route>();f->poll(1000);
        if(nollie){f->poll(1002,100,-1,0,255);f->poll(1004);}
        f->poll(1010,100,-1,a_before_hold?classic_a:0,255);
        for(unsigned at=1044;at<1210;at+=34)f->poll(at,100,-1,a_before_hold?classic_a:0,255);
        f->poll(1210,100,-1,classic_a,255);
        const auto resumed=f->poll(1220,100,-1,classic_a);
        check(resumed.authored_playback&&resumed.requested_id==(nollie?16u:1u)&&resumed.right_y==(nollie?31000:-31000),
              "releasing Pressure while same A remains held resumes exact supported crouch");
        const auto released=f->poll(1230,100,nollie?16:1);
        check(released.jump.setup_admitted&&released.jump.pressed_at==(a_before_hold?1010u:1210u)&&released.jump.nollie==nollie,
              "resumed crouch preserves original physical A timestamp without a synthesized press");
    }
}
void interrupted_pressure_and_missing_recipe() {
    for(bool a_before_hold:{false,true})for(unsigned fault=0;fault<12;++fault) {
        auto f=std::make_unique<Route>();f->poll(1000);f->poll(1010,100,-1,a_before_hold?classic_a:0,255);
        for(unsigned at=1044;at<1210;at+=34)f->poll(at,100,-1,a_before_hold?classic_a:0,255);
        f->poll(1210,100,-1,classic_a,255);
        if(fault==1)++f->owner.entity;
        if(fault==2)f->admission.gameplay=false;
        if(fault==3)f->admission.offboard=true;
        if(fault==4)f->bailing=true;
        if(fault==5)f->admission.external_owner=true;
        if(fault==7)f->contacts=0x4000;
        if(fault==8)f->switched=true;
        if(fault==9)++f->selection_object;
        if(fault==10)f->admission.launch={LaunchFamily::extended,LaunchContext::vertical,0};
        const unsigned at=fault==11?1400u:1220u;
        f->poll(at,100,-1,fault==0?0:classic_a,255,fault==6?255:0);
        f->admission.gameplay=true;f->admission.offboard=false;f->bailing=false;f->admission.external_owner=false;
        f->contacts=0xc000;f->switched=false;f->selection_object=16;f->admission.launch=LaunchDescriptor::ordinary();
        const auto out=f->poll(at+10,100,-1,fault==0?0:classic_a);
        if(out.authored_playback||out.requested_id)std::fprintf(stderr,"interrupted resume a_before=%d fault=%u id=%u\n",a_before_hold,fault,out.requested_id);
        check(!out.authored_playback&&!out.requested_id,"interrupted Pressure setup cannot revive old A after authority returns");
    }
    auto f=std::make_unique<Route>();f->catalog.erase(f->catalog.begin()+1);f->admission.catalog=f->catalog;
    f->poll(1000);check(f->poll(1010,100,-1,classic_a).requested_id==1,"missing-recipe negative first owns normal crouch");
    const auto missing=f->poll(1020,100,1,classic_a,255);
    check(!missing.authored_playback&&!missing.requested_id,"missing exact16 recipe cannot keep ordinary crouch as Nollie substitute");
    check(!f->poll(1030,100,1).jump.setup_admitted,"failed retarget cannot produce an admitted fallback launch");
}
void boundaries_and_grab() {
    for(unsigned fault=0;fault<4;++fault) {
        auto f=std::make_unique<Route>();f->poll(1000);f->poll(1010,100,-1,0,255);f->poll(1020);
        if(fault==0)f->admission.gameplay=false;
        if(fault==1)++f->owner.entity;
        if(fault==2)f->admission.offboard=true;
        if(fault==3)f->bailing=true;
        f->poll(1030);f->admission.gameplay=true;f->admission.offboard=false;f->bailing=false;f->poll(1040);
        check(f->poll(1050,100,-1,classic_a).requested_id==1,"OFF, native owner replacement, bail and offboard clear selected setup");
    }
    auto f=std::make_unique<Route>();f->poll(1000);f->poll(1010,100,-1,0,255);f->poll(1020);
    f->poll(1030,100,-1,classic_a);f->poll(1040,100,16,classic_a);f->poll(1050,100,16);
    f->poll(1060,200,16);f->poll(1070,201,16);
    for(unsigned i=0;i<10;++i)f->poll(1080+i*17,201,16,0x2000);
    check(f->grabs==1&&!f->flips,"Nollie launch supports existing native grab after physical LT release");
}
void aborted_ground_pop() {
    for(bool first_nollie:{false,true}) {
        auto f=std::make_unique<Route>();f->poll(1000);
        if(first_nollie){f->poll(1002,100,-1,0,255);f->poll(1004);}
        f->poll(1010,100,-1,classic_a);f->poll(1020,100,first_nollie?16:1,classic_a);
        check(f->poll(1030,100,first_nollie?16:1).jump.setup_admitted,"aborted-ground fixture owns original pop release");
        for(unsigned at=1040;at<=1600;at+=10)f->poll(at,100,-1);
        f->poll(1610,100,-1,0,255);f->poll(1620,100,-1);
        const auto next=f->poll(1630,100,-1,classic_a);
        check(next.requested_id==(first_nollie?1u:16u)&&next.jump.setup_started&&next.jump.nollie!=first_nollie,
              "fresh LT and A replace a pop which never entered AIR with the newly selected exact native identity");
        const auto release=f->poll(1640,100,first_nollie?1:16);
        check(release.jump.setup_admitted&&release.jump.pressed_at==1630&&release.jump.nollie!=first_nollie,
              "replacement launch has its own A edge and frozen setup rather than the abandoned pop");
    }
}
void pressure_survives_rejected_releases() {
    auto f=std::make_unique<Route>();f->poll(1000);f->poll(1010,100,-1,0,255);
    for(unsigned at=1044;at<1210;at+=34)f->poll(at,100,-1,0,255);
    f->poll(1210,100,-1,classic_a,255);
    check(!f->poll(1220,100,-1,0,255).jump.setup_admitted,"first Pressure A release remains unavailable");
    const auto retry=f->poll(1230,100,-1,classic_a,255);
    check(!retry.authored_playback&&!retry.requested_id&&!retry.jump.setup_started,
          "rejected A release cannot clear still-held Pressure and admit an unrelated Ollie on fresh A");
    const auto resumed=f->poll(1240,100,-1,classic_a);
    check(resumed.requested_id==1&&resumed.authored_playback,"LT release restores supported crouch for the second actual A press");
    check(f->poll(1250,100,1).jump.pressed_at==1230,"Pressure retry recovery retains second physical A identity");
    for(bool first_nollie:{false,true}) {
        auto prior=std::make_unique<Route>();prior->poll(1000);
        if(first_nollie){prior->poll(1002,100,-1,0,255);prior->poll(1004);}
        prior->poll(1010,100,-1,classic_a);prior->poll(1020,100,first_nollie?16:1,classic_a);
        prior->poll(1030,100,first_nollie?16:1);
        for(unsigned at=1040;at<=1600;at+=10)prior->poll(at,100,-1);
        prior->poll(1610,100,-1,0,255);
        for(unsigned at=1644;at<1810;at+=34)prior->poll(at,100,-1,0,255);
        const auto pressure=prior->poll(1810,100,-1,classic_a,255);
        check(!pressure.authored_playback&&!pressure.requested_id,"abandoned launch cannot override prospective Pressure setup");
        const auto recovered=prior->poll(1820,100,-1,classic_a);
        check(recovered.authored_playback&&recovered.requested_id==(first_nollie?16u:1u),
              "Pressure withdrawal after an aborted pop resumes the actual remembered setup");
        const auto release=prior->poll(1830,100,first_nollie?16:1);
        check(release.jump.setup_admitted&&release.jump.pressed_at==1810&&release.jump.nollie==first_nollie,
              "recovered setup replaces old launch and retains its new physical A timestamp");
    }
}
std::unique_ptr<Route> pressure_from_native_crouch(bool nollie,unsigned cadence,bool known_contacts=true) {
    auto f=std::make_unique<Route>();f->omit_contact_proof=!known_contacts;f->poll(1000);
    if(nollie){f->poll(1002,100,-1,0,255);f->poll(1004);}
    const auto pressed=1000+cadence,lt_at=1000+3*cadence;
    const auto pressure_at=lt_at+((PhysicalSetupSelection::pressure_hold_ms+cadence-1)/cadence)*cadence;
    check(f->poll(pressed,100,-1,classic_a).requested_id==(nollie?16u:1u),
          "native-crouch fixture starts the original physical A setup");
    f->poll(1000+2*cadence,103,nollie?16:1,classic_a);
    check(f->poll(lt_at,103,nollie?16:1,classic_a,255).requested_id==(nollie?1u:16u),
          "short LT retargets the already observed native crouch before Pressure qualifies");
    for(auto now=lt_at+cadence;now<pressure_at;now+=cadence)f->poll(now,103,nollie?1:16,classic_a,255);
    const auto pressure=f->poll(pressure_at,103,nollie?1:16,classic_a,255);
    check(!pressure.authored_playback&&!pressure.requested_id&&!pressure.jump.released,
          "Pressure withdraws authored input without fabricating an admitted A release");
    return f;
}
void pressure_retains_observed_native_crouch() {
    for(unsigned cadence:{8u,16u,33u})for(bool nollie:{false,true})
    for(bool release_before_idle:{false,true})for(unsigned frames:{1u,2u}) {
        auto f=pressure_from_native_crouch(nollie,cadence);
        const auto pressure_at=f->admission.native_context.sampled_at;
        for(unsigned index=0;index<frames;++index) {
            const auto waiting=f->poll(pressure_at+(index+1)*cadence,103,nollie?1:16,classic_a,release_before_idle?0:255);
            check(!waiting.authored_playback&&!waiting.requested_id&&!waiting.jump.released,
                  "same native103 actor retains held A but cannot restart crouch before idle100");
        }
        const auto idle_at=pressure_at+(frames+1)*cadence;
        if(!release_before_idle)f->poll(idle_at,100,-1,classic_a,255);
        const auto resumed_at=idle_at+(release_before_idle?0:cadence);
        const auto resumed=f->poll(resumed_at,100,-1,classic_a);
        if(!resumed.authored_playback||resumed.requested_id!=(nollie?16u:1u)||resumed.jump.pressed_at!=1000+cadence)std::fprintf(stderr,"native103 cadence=%u nollie=%d early=%d frames=%u id=%u authored=%d setup=%d press=%llu expected=%u\n",cadence,nollie,release_before_idle,frames,resumed.requested_id,resumed.authored_playback,resumed.jump.setup_started,static_cast<unsigned long long>(resumed.jump.pressed_at),1000+cadence);
        check(resumed.authored_playback&&resumed.requested_id==(nollie?16u:1u)&&resumed.jump.pressed_at==1000+cadence,
              "native103 withdrawal resumes exact supported setup at idle100 under original physical A");
        const auto release=f->poll(resumed_at+cadence,100,nollie?16:1);
        check(release.jump.setup_admitted&&release.jump.pressed_at==1000+cadence&&release.jump.released_at==resumed_at+cadence&&
              release.jump.nollie==nollie,"resumed native crouch preserves A edge timestamps and launch identity");
    }
    for(bool nollie:{false,true})for(unsigned fault=0;fault<15;++fault) {
        auto f=pressure_from_native_crouch(nollie,16,fault!=12);
        const auto pressure_at=f->admission.native_context.sampled_at;
        if(fault==2)++f->owner.entity;
        if(fault==3)f->contacts=0x4000;
        if(fault==5) {
            // Native publication sees another actor between controller polls;
            // returning to the same id cannot erase the action generation.
            NativeRequestSample foreign{};foreign.valid=true;foreign.state=103;foreign.crouch_mode=1;
            foreign.words[1]=f->contacts;
            (void)f->producer.observe(pressure_at+8,f->owner,foreign,{true,2,f->selection_object});
        }
        if(fault==6)++f->selection_object;
        if(fault==7)f->bailing=true;
        if(fault==8)f->admission.gameplay=false;
        if(fault==9)f->admission.external_owner=true;
        f->poll(pressure_at+16,fault==1?200u:103u,fault==4?2:nollie?1:16,fault==0?0:classic_a,255);
        f->contacts=0xc000;f->selection_object=16;f->bailing=false;
        f->admission.gameplay=true;f->admission.external_owner=false;
        // A state and contact-mask change in one native callback increments
        // contact generation only once. The actual mask must distinguish it
        // from a genuine same-contact crouch retirement.
        if(fault==10)f->contacts=0x4000;
        f->omit_contact_proof=fault==11;
        if(fault==13)f->forged_idle_generations=f->admission.native_context.generations;
        f->poll(pressure_at+32,100,fault==14?2:-1,fault==0?0:classic_a,255);
        const auto resumed=f->poll(pressure_at+48,100,fault==14?2:-1,fault==0?0:classic_a);
        check(!resumed.authored_playback&&!resumed.requested_id&&!resumed.jump.released,
              "released A, AIR, foreign actor/action, owner/contact/object loss, bail, OFF, external owner or missing contact proof cancels crouch");
    }
}
}
int main(){pressure_retains_observed_native_crouch();launch_selection();pressure_refuses_substitution();boundaries_and_grab();held_retarget_and_second_tap();pressure_release_resumes_same_a();interrupted_pressure_and_missing_recipe();aborted_ground_pop();pressure_survives_rejected_releases();std::printf("setup session: %u checks passed\n",checks);}
