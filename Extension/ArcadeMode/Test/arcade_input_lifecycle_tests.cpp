#include "Extension/ArcadeMode/arcade_input_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* reason) {
    ++checks;if(!value){std::cerr<<"FAIL: "<<reason<<'\n';std::exit(1);}
}
InputNativeContext frame(std::uint64_t now,InputGenerations generations={1,1,1}) {
    return {true,true,generations,now+1,now,false,true,1,0};
}
struct Receiver {
    unsigned calls{};InputRequest last{};InputDisposition result{InputDisposition::waiting};
    static InputDisposition request(void* state,const InputRequest& request) noexcept {
        auto& self=*static_cast<Receiver*>(state);++self.calls;self.last=request;return self.result;
    }
    InputAdapter adapter(){return {this,request};}
};
void producer_contract() {
    struct Published {
        bool valid{true},rail_family{true};
        std::uint64_t owner_generation{4},contact_generation{5},action_generation{6},physics_step{7},sampled_at{10};
    } published;
    const auto native=InputNativeContext::from_frame(published);
    check(native.required && native.valid && native.rail_family && native.generations==InputGenerations{4,5,6} &&
        native.physics_step==7 && native.sampled_at==10,"native conversion preserves the actual producer correlation without native identity inference");
    for(unsigned age:{149u,150u,151u})check(native.fresh(10+age)==(age<=150),"the producer lease is inclusive150ms, not a renewable input-poll clock");
    check(!native.fresh(9),"future native samples cannot admit an earlier physical poll");
    for(unsigned field=0;field<4;++field){auto invalid=native;if(field==0)invalid.generations.owner=0;if(field==1)invalid.generations.contact=0;if(field==2)invalid.generations.action=0;if(field==3)invalid.physics_step=0;check(!invalid.fresh(10),"zero correlation fields cannot manufacture a native frame");}
    check(native.follows(native),"multiple polls may read exactly the same physics frame");
    auto changed=native;++changed.sampled_at;check(!changed.follows(native),"the same physics frame cannot renew its timestamp");
    changed=native;++changed.generations.action;check(!changed.follows(native),"generation changes require a new physics frame");
    changed=native;--changed.physics_step;check(!changed.follows(native),"physics step rollback invalidates native continuity");
    changed=native;++changed.physics_step;--changed.generations.contact;check(!changed.follows(native),"contact generation rollback cannot re-admit old context");
    changed=native;++changed.physics_step;++changed.generations.owner;check(changed.follows(native),"a new owner generation establishes a new hard boundary");
    changed=native;--changed.generations.owner;check(!changed.follows(native),"an older owner generation cannot replace the current authority");
    changed=native;++changed.generations.owner;check(!changed.follows(native),"owner reacquisition cannot manufacture a new correlation without a new physics frame");
}
void generation_bound_queue() {
    struct Rule {InputAction action;std::uint8_t mask;};
    const Rule rules[]{{InputAction::manual,1},{InputAction::ollie_release,1},{InputAction::flip,5},
        {InputAction::grab,5},{InputAction::rail_change,7},{InputAction::revert,7}};
    for(const auto rule:rules)for(unsigned changed=0;changed<3;++changed) {
        InputRequestQueue queue;Receiver receiver;InputRequest request{rule.action,InputContext::ground,10,0,1};
        request.unbounded=true;request.generations={1,1,1};request.generation_mask=rule.mask;queue.push(request);
        unsigned cancelled{};const auto report=[&](InputDispatchEvent event){cancelled+=event.disposition==InputDisposition::cancelled;};
        queue.dispatch(20,1,InputContext::ground,receiver.adapter(),report,frame(20));
        check(receiver.calls==1 && receiver.last.recognized_at==10,"a waiting native callback retains its exact recognition origin");
        auto next=frame(30);if(changed==0)++next.generations.owner;if(changed==1)++next.generations.contact;if(changed==2)++next.generations.action;
        receiver.result=InputDisposition::started;
        queue.dispatch(30,1,InputContext::ground,receiver.adapter(),report,next);
        const bool invalidated=(rule.mask&(1u<<changed))!=0;
        check(receiver.calls==(invalidated?1u:2u) && cancelled==(invalidated?1u:0u),"generation masks cancel stale native work before callbacks without imposing a handoff deadline");
    }
}
void recognition_and_consumption() {
    for(unsigned cadence:{17u,34u})for(unsigned phase=0;phase<cadence;++phase)for(bool pad:{false,true}) {
        UnifiedInput input;Receiver receiver;InputEnvironment environment{InputContext::ground,true};
        environment.manual_safe=false;environment.manual_prepare=true;
        std::uint64_t recognized{};
        for(unsigned now=phase;now<500;now+=cadence) {
            environment.native_context=frame(now);
            const int direction=now<100?0:now<300?1:-1;
            const PhysicalInputSample physical{static_cast<std::uint16_t>(pad?(direction>0?1:direction<0?2:0):0),0,
                static_cast<std::int16_t>(pad?0:direction*25000)};
            input.poll(now,physical,environment,receiver.adapter());
            if(!recognized && receiver.calls)recognized=receiver.last.recognized_at;
        }
        check(recognized>=300 && recognized<300+cadence && receiver.last.recognized_at==recognized,
            "normal thumb traces at every30/60Hz phase retain a single Manual recognition timestamp");
        check(receiver.last.generation_mask==1 && receiver.last.generations.owner==1,
            "Manual preparation binds owner without treating takeoff/landing counters as expiration");
        bool consumed{};for(const auto& event:input.history().events())consumed=consumed || (event.owners&InputConsumption::manual);
        check(consumed,"the actual Manual path commits its category footprint transactionally");
    }
    for(bool grab:{false,true}) {
        UnifiedInput input;Receiver receiver;InputEnvironment environment{InputContext::air,true};
        environment.native_context=frame(0);input.poll(0,{},environment,receiver.adapter());
        environment.native_context=frame(10);input.poll(10,{0,-25000},environment,receiver.adapter());
        environment.native_context=frame(20);input.poll(20,{static_cast<std::uint16_t>(grab?0x2000:0x4000),-25000},environment,receiver.adapter());
        check(receiver.calls==1 && receiver.last.generation_mask==5 && receiver.last.recognized_at==20,
            "ordinary Flip/Grab recognition binds the native action generation");
        bool consumed{};for(const auto& event:input.history().events())consumed=consumed || (event.owners&InputConsumption::regular);
        check(consumed,"ordinary air requests share the named regular consumption category");
        environment.native_context=frame(30,{1,2,1});input.poll(30,{},environment,receiver.adapter());
        check(receiver.calls==2 && receiver.last.recognized_at==20,"a contact-only counter change cannot delete a recognized ordinary air request");
        receiver.result=InputDisposition::started;environment.native_context=frame(40,{1,2,2});input.poll(40,{},environment,receiver.adapter());
        check(receiver.calls==2 && input.diagnostic().dispatched==1 && input.diagnostic().dispatch[0].disposition==InputDisposition::cancelled,
            "native action changes cancel the stale waiting air request before newly available readiness");
    }
}
void thumb_travel_and_authority() {
    for(unsigned cadence:{17u,34u})for(unsigned phase=0;phase<cadence;++phase)
    for(bool diagonal:{false,true})for(bool late:{false,true})for(bool owner_gap:{false,true}) {
        UnifiedInput input;Receiver receiver;receiver.result=InputDisposition::started;
        InputEnvironment environment{InputContext::ground,true};
        std::uint64_t first{},second{};
        for(unsigned now=phase;now<800;now+=cadence) {
            const auto generations=owner_gap && now>=200?InputGenerations{2,2,2}:InputGenerations{1,1,1};
            environment.native_context=frame(now,generations);
            const unsigned opposite=late?500u:300u;
            const bool forward=now>=100 && now<170;
            const bool arc=diagonal && now>=170 && now<220;
            if(!first && forward)first=now;if(!second && now>=opposite)second=now;
            input.poll(now,{static_cast<std::uint16_t>(now<100?0x2000:0),
                static_cast<std::int16_t>(arc?20000:0),static_cast<std::int16_t>(forward?25000:arc?20000:now>=opposite?-25000:0)},
                environment,receiver.adapter());
        }
        check(receiver.calls==(!late && !owner_gap?1u:0u),
            "B release with the first direction survives normal neutral/diagonal thumb travel; stale timing and owner-gap negatives remain rejected");
        const auto expected_first=phase+((100+cadence-1-phase)/cadence)*cadence;
        const auto expected_second=phase+(((late?500u:300u)+cadence-1-phase)/cadence)*cadence;
        check(first==expected_first && second==expected_second && ((second-first<=275)==!late),
            "thumb trace expectations use sampled physical edge times rather than a widened recognition deadline");
    }
}
void jump_contact_continuity() {
    UnifiedInput input;Receiver receiver;receiver.result=InputDisposition::started;
    InputEnvironment environment{InputContext::ground,true};environment.native_context=frame(0);input.poll(0,{},environment,receiver.adapter());
    environment.native_context=frame(10);input.poll(10,{0x1000},environment,receiver.adapter());
    environment.context=InputContext::air;environment.ollie_allowed=false;environment.native_context=frame(20,{1,2,2});
    input.poll(20,{},environment,receiver.adapter());
    check(receiver.calls==2 && receiver.last.action==InputAction::ollie_release && receiver.last.generation_mask==1 &&
        receiver.last.physical_pressed_at==10 && receiver.last.physical_released_at==20,
        "an admitted physical Ollie release survives legitimate takeoff contact/action changes without retimestamping");
}
void parent_table_generation() {
    UnifiedInput input;Receiver receiver;receiver.result=InputDisposition::started;
    InputEnvironment environment{InputContext::air,true};
    environment.native_context=frame(0);input.poll(0,{},environment,receiver.adapter());
    environment.native_context=frame(10);input.poll(10,{0,-25000},environment,receiver.adapter());
    environment.native_context=frame(20);input.poll(20,{0x4000,-25000},environment,receiver.adapter());
    check(receiver.calls==1 && receiver.last.presses==1,"ordinary action establishes the parent table correlation");
    environment.native_context=frame(30);input.poll(30,{0,-25000},environment,receiver.adapter());
    environment.native_context=frame(40,{1,1,2});input.poll(40,{0x4000,-25000},environment,receiver.adapter());
    const auto& diagnostic=input.diagnostic();
    check(receiver.calls==1 && diagnostic.dispatched==1 && diagnostic.dispatch[0].request.presses==2 &&
        diagnostic.dispatch[0].disposition==InputDisposition::cancelled && diagnostic.dispatch[0].request.generations.action==1,
        "a stale parent extra table cancels its repeated request rather than dispatching a simpler native action");
}
void manual_landing_and_revert() {
    InputSession session;InputAdmission admission{};admission.gameplay=admission.manual_prepare=true;
    admission.pose=SkatePose::air;admission.native_context=frame(0);session.poll(0,{},0,0,admission);
    admission.native_context=frame(10);session.poll(10,{0,0,25000},0,0,admission);
    admission.native_context=frame(110);const auto queued=session.poll(110,{0,0,-25000},0,0,admission);
    check(!queued.manual.owns && session.pending_manual().recognized_at==110,"air Manual recognition waits without acquiring native channels");
    admission.native_context=frame(120,{1,2,2});admission.pose=SkatePose::riding;admission.manual_safe=true;
    const auto landed=session.poll(120,{0,0,-25000},0,0,admission);
    check(landed.manual.owns && landed.manual.request_at==120,"fresh landing generations admit the queued Manual and start the separate native landing-settling clock");
    admission.native_context=frame(130,{1,3,3});admission.pose=SkatePose::air;
    const auto rebound=session.poll(130,{0,0,-25000},0,0,admission);
    check(rebound.manual.request_at==120,"a short native rebound cannot renew the existing bounded landing-settling clock");
    admission.native_context=frame(140,{1,4,4});admission.native_context.rail_family=true;
    check(!session.poll(140,{0,0,-25000},0,0,admission).manual.owns,"verified rail contact cancels the native Manual owner independently of HUD pose");
    UnifiedInput input;Receiver receiver;InputEnvironment environment{InputContext::ground,true};
    environment.native_context=frame(0);input.poll(0,{},environment,receiver.adapter());
    environment.native_context=frame(10);input.poll(10,{0,0,0,0,255},environment,receiver.adapter());
    check(receiver.calls==1 && receiver.last.action==InputAction::revert && receiver.last.generation_mask==7,
        "the existing Revert path binds owner, contact and action freshness");
    receiver.result=InputDisposition::started;environment.native_context=frame(20,{1,2,1});
    input.poll(20,{0,0,0,0,255},environment,receiver.adapter());
    check(receiver.calls==1 && input.diagnostic().dispatch[0].disposition==InputDisposition::cancelled,
        "contact loss cancels a waiting Revert before callback readiness can actuate it");
    environment.native_context=frame(30,{1,2,1});input.poll(30,{},environment,receiver.adapter());
    environment.native_context=frame(40,{1,2,1});input.poll(40,{0,0,0,0,255},environment,receiver.adapter());
    check(receiver.calls==2 && receiver.last.recognized_at==40,"only a genuinely new Revert press rearms after contact changes");
    bool consumed{};for(const auto& event:input.history().events())if(event.token.source==InputSource::triggers)
        consumed=consumed || (event.owners&InputConsumption::regular);
    check(consumed,"Revert uses the same regular consumption category as ordinary actions");
}
void observed_branch_transitions() {
    InputSession session;Receiver receiver;receiver.result=InputDisposition::started;
    InputAdmission admission{};admission.gameplay=admission.manual_safe=admission.manual_balance_admitted=true;
    admission.pose=SkatePose::manual;admission.extensions=receiver.adapter();
    auto poll=[&](std::uint64_t now,std::uint16_t buttons,unsigned branch,std::uint64_t generation){
        admission.observed_balance_branch=branch;admission.native_context=frame(now,{1,1,generation});
        admission.balance_generations={1,1,generation};
        admission.balance_controls=InputBalanceControls::vertical();admission.balance_controls.generations={1,1,generation};
        return session.poll(now,{buttons},0,0,admission);
    };
    poll(0,0,0,1);poll(10,0x4000,0,1);poll(20,0,0,1);poll(30,0x8000,0,1);
    check(receiver.calls==1 && receiver.last.variant==2 && receiver.last.balance_origin==0 && receiver.last.generation_mask==7,
        "the shared basic balance table emits a generation-bound first branch request");
    poll(40,0,0,1);poll(50,0x4000,0,1);poll(60,0,0,1);poll(70,0x8000,0,1);
    check(receiver.calls==2 && receiver.last.variant==2 && receiver.last.balance_origin==0,
        "successful adapter admission cannot manufacture the observed branch or turn a repeated request into its return transition");
    poll(80,0,2,2);poll(90,0x8000,2,2);poll(100,0,2,2);poll(110,0x8000,2,2);
    check(receiver.calls==3 && receiver.last.variant==6 && receiver.last.balance_origin==2,
        "an independently observed first branch installs the existing shared second branch recognition");
    poll(120,0,6,3);poll(130,0x8000,6,3);poll(140,0,6,3);poll(150,0x8000,6,3);
    check(receiver.calls==4 && receiver.last.action==InputAction::balance_return && receiver.last.variant==0 && receiver.last.balance_origin==6,
        "an independently observed second branch uses the shared return rule without a separate recognizer");
    InputSession unsupported;admission.extensions={};admission.observed_balance_branch=0;
    admission.balance_generations={1,1,1};
    admission.native_context=frame(0);unsupported.poll(0,{},0,0,admission);
    admission.native_context=frame(10);unsupported.poll(10,{0x4000},0,0,admission);
    admission.native_context=frame(20);const auto missing=unsupported.poll(20,{0x8000},0,0,admission);
    check(missing.unsupported && !missing.action.owned && !missing.authored_playback,
        "a recognized branch with no native extension remains explicitly unsupported and owns no playback");
}
void typed_controls_and_rail_requests() {
    UnifiedInput input;Receiver receiver;InputEnvironment environment{InputContext::rail,true};
    environment.native_context=frame(0);environment.balance_controls={InputBalanceKind::grind,
        {InputSource::direction,4,InputEdge::press},{InputSource::direction,6,InputEdge::press},true,{1,1,1}};
    environment.rail={InputRailPhase::established,1,100,0,0,0,true,true,true,false,false,false,true,true};
    environment.rail.generations={1,1,1};
    input.poll(0,{},environment,receiver.adapter());environment.native_context=frame(10);input.poll(10,{4},environment,receiver.adapter());
    check(input.history().events().empty(),"fresh typed rail controls suppress command events without suppressing physical held state");
    environment.native_context=frame(20,{1,2,1});input.poll(20,{},environment,receiver.adapter());
    environment.native_context=frame(30,{1,2,1});input.poll(30,{8},environment,receiver.adapter());
    check(!input.history().events().empty(),"old-contact control publication cannot suppress commands after the preceding-publication interval");
    environment.rail.generations={1,2,1};
    environment.native_context=frame(40,{1,2,1});input.poll(40,{0x4000},environment,receiver.adapter());
    environment.native_context=frame(50,{1,2,1});input.poll(50,{},environment,receiver.adapter());
    environment.native_context=frame(60,{1,2,1});input.poll(60,{0x2000},environment,receiver.adapter());
    check(receiver.calls==1 && receiver.last.action==InputAction::rail_change && receiver.last.generation_mask==7,
        "existing shared rail definitions bind branch dispatch to contact and action generations");
    bool consumed{};for(const auto& event:input.history().events())consumed=consumed || (event.owners&InputConsumption::extra);
    check(consumed,"contextual rail transitions use the shared extra consumption category");
    receiver.result=InputDisposition::started;environment.native_context=frame(70,{1,3,1});input.poll(70,{},environment,receiver.adapter());
    check(receiver.calls==1 && input.diagnostic().dispatch[0].disposition==InputDisposition::cancelled,
        "contact loss invalidates the pending rail transition even if a stale rail phase remains published");
}
void session_authority() {
    for(unsigned loss=0;loss<5;++loss) {
        InputSession session;InputAdmission admission{};admission.gameplay=admission.action_fresh=admission.inspected_stance=true;
        admission.pose=SkatePose::air;admission.action_mode=ActionExperiment::classic;admission.native_context=frame(0);
        session.poll(0,{},0,0,admission);admission.native_context=frame(10);
        check(session.poll(10,{0x2000},0,0,admission).action.owned,"native-correlated held grab uses the existing adapter");
        admission.native_context=loss==0?frame(20,{2,2,2}):frame(20);
        if(loss==1)admission.native_context.valid=false;
        if(loss==2)admission.native_context=frame(10);
        if(loss==3)admission.native_context={};
        if(loss==4){admission.native_context=frame(10);admission.native_context.sampled_at=20;}
        const std::uint64_t now=loss==2?161:20;
        if(loss==2)check(session.poll(160,{0x2000},0,0,admission).action.owned,"the actual session retains valid native ownership at the exact150ms producer lease endpoint");
        const auto out=session.poll(now,{0x2000},123,456,admission);
        check(!out.action.owned && !session.busy(),"owner loss, invalid/stale/absent frames and frame retimestamping clear native owners");
        check(session.diagnostic().dispatched==0,"authority loss cannot leave an earlier successful dispatch published as a current input result");
        if(loss!=0)check(out.buttons==0x2000 && out.right_x==123 && out.right_y==456,"untrusted native context restores exact physical channels");
        admission.native_context=frame(180,{2,2,2});
        check(!session.poll(180,{0x2000},0,0,admission).action.owned,"held-through-authority-gap input cannot become a fresh press");
        admission.native_context=frame(190,{2,2,2});session.poll(190,{},0,0,admission);
        admission.native_context=frame(200,{2,2,2});check(session.poll(200,{0x2000},0,0,admission).action.owned,"a genuinely fresh press can rearm after authority returns");
    }
    InputSession rail;InputAdmission admission{};admission.gameplay=admission.may_start=admission.may_push=admission.manual_safe=true;
    admission.pose=SkatePose::riding;admission.native_context=frame(0);admission.native_context.rail_family=true;
    rail.poll(0,{},0,0,admission);admission.native_context=frame(10);admission.native_context.rail_family=true;
    const auto out=rail.poll(10,{0,25000},0,0,admission);
    check(!(out.buttons&0x4000u) && !out.authored_playback,"verified physical rail family blocks stale ground actuation without inventing named rail identities");
}
}
int main(){producer_contract();generation_bound_queue();recognition_and_consumption();thumb_travel_and_authority();jump_contact_continuity();manual_landing_and_revert();parent_table_generation();observed_branch_transitions();typed_controls_and_rail_requests();session_authority();std::cout<<checks<<" input lifecycle checks passed\n";}
