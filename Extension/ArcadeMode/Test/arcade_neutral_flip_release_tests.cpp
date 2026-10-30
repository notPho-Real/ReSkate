#include "Extension/ArcadeMode/arcade_gesture.h"
#include "arcade_native_flip_entry_tests.h"
#include "Extension/ArcadeMode/arcade_native_readiness.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* text) { ++checks;if(!value){std::cerr<<"FAIL: "<<text<<'\n';std::exit(1);} }
struct Route {
    ArcadeInput input{true}; NativeInputState producer; NativeInputReadiness readiness;
    NativeInputOwner owner{1,2,3,4,5}; NativeInputFrame frame{};
    std::vector<PreparedGesture> recipes;
    unsigned starts{},recognized{}; bool exact_clock{};NativeFlipRequest normal_flip{};
    NativeGesture::Output output{};
    Route() {
        for(const unsigned id:{1u,2u,3u,4u,5u,6u,8u,9u,36u})
            recipes.emplace_back(static_cast<std::uint8_t>(id),false,GestureContext::ground,
                std::vector<AuthoredStickStep>{{0,0,-32767},{18,28000,28000},{21,0,0},{24,0,0}});
        input.enable(true);input.set_recognition_capture(0,true);
    }
    NativeGesture::Output poll(std::uint64_t now,PhysicalInputSample pad={},std::uint32_t state=201,
            std::int32_t selection=-1,std::uint32_t mode=2,bool gameplay=true) {
        NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=mode;
        sample.words[1]=state==100?0xc000u:0u;
        frame=producer.observe(now,owner,sample,{true,selection,16});
        readiness.publish(frame,{true,state==100&&mode==1,false,false,false,false,false,false,true,0});
        input.publish_native_context(frame);input.publish_authority(now,frame.owner_generation,1,true);
        const auto admit=readiness.sample(now,frame);
        output=input.poll(0,now,pad.buttons,pad.x,pad.y,0,0,gameplay,admit.start,false,0,false,false,
            true,exact_clock?now*60/1000:now/17,false,recipes,admit.mirror_x,false,frame.airborne?SkatePose::air:SkatePose::riding,
            false,false,false,false,admit.flip_ready,false,0,0,ActionExperiment::classic,admit.action,admit.mirror_x,false,normal_flip);
        starts+=output.requested_id?1u:0u;
        const auto* diagnostic=input.input_diagnostic(0);
        recognized+=static_cast<unsigned>(diagnostic->recognition_count);
        return output;
    }
};
void neutral_once() {
    for(unsigned cadence:{17u,34u}) {
        auto h=std::make_unique<Route>();h->poll(1000);
        const auto out=h->poll(1000+cadence,{0x4000});
        check(out.requested_id==2 && out.authored_playback,"production neutral X press immediately requests the basic Kickflip adapter");
        const auto* d=h->input.input_diagnostic(0);check(d->recognition_count==1,"neutral press recognizes one immutable request");
        const auto request=d->recognition[0].request;
        check(request.form==InputForm::neutral_press && request.direction==0 &&
            request.air.trick==ArcadeTrick::kickflip && request.air.predicate==AirInputPredicate::face,
            "default Kickflip uses explicit neutral-press semantics");
        check(request.physical_pressed_at==1000+cadence && !request.physical_released_at &&
            request.recognized_at==1000+cadence && request.recognition && request.recognition->footprint.count==1,
            "recognition preserves the original press without waiting for or inventing release");
        check(!h->poll(1000+2*cadence).requested_id,"release cannot repeat a consumed neutral press");
        for(unsigned n=3;n<10;++n)h->poll(1000+std::uint64_t{n}*cadence);
        check(h->starts==1,"neutral release cannot repeat from held state, later polls or its consumed face");
    }
}
void directional_precedence() {
    struct Choice {std::uint16_t direction;unsigned id;};
    for(const auto choice:{Choice{4,2},Choice{8,3},Choice{1,36},Choice{2,4},Choice{5,9},Choice{9,6},Choice{6,5},Choice{10,8}})
    for(bool simultaneous:{false,true}) {
        auto h=std::make_unique<Route>();h->poll(1000);
        h->poll(1020,{static_cast<std::uint16_t>(simultaneous?0:choice.direction)});
        auto out=h->poll(1040,{static_cast<std::uint16_t>(choice.direction|0x4000u)});
        check(out.requested_id==choice.id,"direction before or in the release poll selects its existing exact native variant");
        h->poll(1060);check(h->starts==1,"release following a consumed directional face never adds a default Kickflip");
    }
    for(bool released:{false,true}) {
        auto h=std::make_unique<Route>();h->input.set_input_policy(InputPolicy::compatibility());h->poll(1000);
        h->poll(1020,{0x4000});if(released)h->poll(1040);
        for(std::uint64_t now=1100;now<1500;now+=100)h->poll(now,{static_cast<std::uint16_t>(released?0u:0x4000u)});
        const auto out=h->poll(1519,{8});
        check(out.requested_id==3,"compatibility retains the old released/held face499ms directional query");
    }
}
void native_wait_and_ollie() {
    auto h=std::make_unique<Route>();h->poll(1000,{},200,1);
    h->poll(1020,{0x4000},200,1);auto out=h->poll(1040,{},200,1);
    check(!out.authored_playback && out.command_busy,"neutral release cannot bypass the independently observed native jump selection");
    const auto original=h->input.input_diagnostic(0)->recognition[0].request;
    h->poll(1060,{},201,1);out=h->poll(1080);
    check(out.requested_id==2 && h->starts==1,"same-flight forward phase and native idle release the original neutral request");
    check(h->input.input_diagnostic(0)->dispatch[0].request==original,"native readiness never renews or rewrites the original release request");

    auto jump=std::make_unique<Route>();jump->poll(1000,{},100,-1,1);
    check(jump->poll(1017,{0x1000},100,-1,1).requested_id==1,"ordinary A reaches the existing authored Ollie setup");
    jump->poll(1051,{0x1000},100,1,0);jump->poll(1085,{},100,1,0);
    jump->poll(1102,{},200,1);jump->poll(1119,{},201,1);jump->poll(1136,{},201,1);
    out=jump->poll(1153,{0x4000},201,1);
    check(out.requested_id==2,"early physical X starts while the owned native Ollie remains selected");
    jump->poll(1170,{},201,1);out=jump->poll(1187);
    check(out.authored_playback && jump->starts==2,
        "actual production A release, native flight and neutral X release hand off to the existing Kickflip actor once");
}
void cancellation() {
    for(unsigned fault=0;fault<7;++fault) {
        auto h=std::make_unique<Route>();h->poll(1000);h->poll(1020,{0x4000});
        if(fault==0)++h->owner.rig;
        if(fault==1){h->input.enable(false);h->input.enable(true);}
        if(fault==2)h->poll(1040,{0x4000},100,-1,1);
        if(fault==3)h->poll(1040,{0x4000},201,-1,2,false);
        if(fault==4)h->poll(1040,{0x4000},300);
        if(fault==5)h->poll(1040,{0x400cu});
        if(fault==6)h->poll(1040,{0x6000});
        const auto out=h->poll(1060);
        check(!out.requested_id,"owner/OFF/landing/authority/bail/opposing/newer-input boundary prevents stale neutral release");
    }
    for(unsigned age:{499u,500u,501u}) {
        auto h=std::make_unique<Route>();h->poll(1000);h->poll(1020,{0x4000});
        for(std::uint64_t now=1100;now<1520;now+=100)h->poll(now,{0x4000});
        const auto out=h->poll(1020+age);
        check(!out.requested_id&&h->starts==1,"held neutral X and delayed release never repeat the original press");
    }
    auto ground=std::make_unique<Route>();ground->poll(1000,{},100,-1,1);ground->poll(1020,{0x4000},100,-1,1);
    check(!ground->poll(1040).requested_id,"ground face press is never borrowed by a later airborne release");
    auto startup=std::make_unique<Route>();startup->poll(1000,{0x4000});
    check(!startup->poll(1020).requested_id,"held startup X must release before any fresh command can arm");
    auto cached=std::make_unique<Route>();cached->poll(1000);cached->poll(1020,{8});cached->poll(1040);
    for(std::uint64_t now=1100;now<=1600;now+=100)cached->poll(now);
    const auto out=cached->poll(1620,{0x4000});
    check(out.requested_id==2 && cached->input.input_diagnostic(0)->recognition[0].request.direction==0,
        "stale cached Right cannot turn neutral release into a Heelflip or fictional direction");
}
void release_order() {
    for(bool simultaneous:{false,true}) {
        auto h=std::make_unique<Route>();h->poll(1000,{0x1000});
        const auto first=h->poll(1020,{0x5000});
        if(!simultaneous)h->poll(1040,{0x4000});
        const auto out=h->poll(1060);
        check(first.requested_id==2&&!out.requested_id,"A release order cannot delay or duplicate the original airborne X press");
    }
}
void queued_withdrawal() {
    for(unsigned fault=0;fault<5;++fault) {
        auto h=std::make_unique<Route>();h->poll(1000,{},200,1);h->poll(1020,{0x4000},200,1);
        check(h->poll(1040,{},200,1).command_busy && h->starts==0,
            "new neutral form waits for native idle before a withdrawal test");
        if(fault==0)++h->owner.rig;
        if(fault==1)h->poll(1060,{},100,-1,1);
        if(fault==2){h->input.enable(false);h->input.enable(true);}
        if(fault==3)h->poll(1060,{},201,1,0);
        if(fault==4)h->poll(1060,{},201,3);
        h->poll(1080);
        check(h->starts==0,"queued neutral release cannot outlive owner/flight/OFF/mode or another native selection");
    }
}
void prompt_directional_tap() {
    struct Choice {std::uint16_t pad;unsigned native;};
    for(const auto choice:{Choice{4,2},Choice{8,3},Choice{2,4}})
    for(unsigned cadence:{17u,34u}) for(bool before:{false,true}) for(bool release_direction:{false,true}) {
        auto h=std::make_unique<Route>();h->poll(1000,{},100,-1,1);
        h->poll(1000+cadence,{0x1000},100,-1,1);
        h->poll(1000+2*cadence,{0x1000},100,1,0);
        const auto release=1000+3*cadence;h->poll(release,{},100,1,0);
        unsigned flips{};std::uint64_t pressed{};
        for(unsigned n=1;n<=15;++n) {
            const bool direction=n>=(before?1u:2u) && (!release_direction || n<5);
            const auto buttons=static_cast<std::uint16_t>((n>=2 && n<4?0x4000u:0u)|(direction?choice.pad:0u));
            const auto out=h->poll(release+n*cadence,{buttons},n<4?103u:n==4?200u:201u,n<8?1:-1,n<=4?0u:2u);
            if(out.requested_id && out.requested_id!=1) {
                if(out.requested_id!=choice.native)std::cerr<<"wanted "<<choice.native<<" got "<<out.requested_id<<" before "<<before<<" release "<<release_direction<<'\n';
                check(out.requested_id==choice.native,"prompt direction before/during X selects the original basic variant");
                check(h->input.authored_air_followup(release+n*cadence,h->frame)==static_cast<std::int32_t>(choice.native),"admitted basic directional flip retains its original flight trajectory identity");++flips;
            }
            const auto* d=h->input.input_diagnostic(0);
            for(std::size_t i=0;i<d->recognition_count;++i)if(d->recognition[i].request.action==InputAction::flip)
                pressed=d->recognition[i].request.physical_pressed_at;
        }
        check(flips==1,"prompt directional tap survives native pop transition once");
        check(pressed==release+2*cadence,"directional prompt flip keeps the original X press time");
    }
}
void prompt_released_ollie_tap() {
    // The accepted hardware control pressed X404ms after A release. These
    // missing-function cases press/release before, across and just after AIR.
    for(unsigned cadence:{17u,34u}) for(unsigned press_step:{0u,1u,3u,5u}) {
        auto h=std::make_unique<Route>();h->poll(1000,{},100,-1,1);
        h->poll(1000+cadence,{0x1000},100,-1,1);
        h->poll(1000+2*cadence,{0x1000},100,1,0);
        const auto release=1000+3*cadence;h->poll(release,{static_cast<std::uint16_t>(press_step==0?0x4000:0)},100,1,0);
        unsigned flip_starts{}, first_flip_step{};std::uint64_t original_press{},original_release{};
        if(press_step==0)for(const auto& attempt:h->input.input_diagnostic(0)->recognition)
            if(attempt.request.action==InputAction::flip)original_press=attempt.request.physical_pressed_at;
        for(unsigned n=1;n<=15;++n) {
            const auto now=release+n*cadence;
            const auto state=n<4?103u:n==4?200u:201u;
            const auto mode=n<=4?0u:2u;
            const bool held=n>=press_step && n<press_step+2;
            const auto out=h->poll(now,{static_cast<std::uint16_t>(held?0x4000:0)},state,n<8?1:-1,mode);
            if(out.requested_id==2){++flip_starts;first_flip_step=n;}
            const auto* d=h->input.input_diagnostic(0);
            for(std::size_t i=0;i<d->recognition_count;++i)if(d->recognition[i].request.action==InputAction::flip) {
                original_press=d->recognition[i].request.physical_pressed_at;
                original_release=d->recognition[i].request.physical_released_at;
            }
            if(n<4)check(out.requested_id!=2,"prompt tap cannot execute before real native AIR");
        }
        check(flip_starts==1 && first_flip_step<8,"owned Ollie hands off during native jump selection, before retirement, exactly once");
        check(original_press==release+press_step*cadence && (original_release==0 || original_release==release+(press_step+2)*cadence),
            "early recognition retains the real press and never invents a future release");
    }
    for(bool native_route:{false,true}) for(unsigned fault=0;fault<12;++fault) {
        auto h=std::make_unique<Route>();unsigned submissions{};
        if(native_route)h->normal_flip={&submissions,[](void* context,std::int32_t) noexcept {
            ++*static_cast<unsigned*>(context);return true;
        }};
        h->poll(1000,{},100,-1,1);h->poll(1017,{0x1000},100,-1,1);
        h->poll(1051,{0x1000},100,1,0);h->poll(1085,{},100,1,0);
        h->poll(1102,{0x4000},103,1,0);h->poll(1119,{},103,1,0);
        if(fault==0)++h->owner.rig;
        if(fault==1){h->input.enable(false);h->input.enable(true);}
        if(fault==2)h->poll(1136,{},103,1,0,false);
        if(fault==3)h->poll(1136,{},300,1,0);
        if(fault==4)h->poll(1136,{0x8000},103,1,0);
        if(fault==5)h->poll(1136,{},103,2,0);
        if(fault==6)h->poll(1136,{},100,1,0); // Reversed native phase.
        if(fault==7){for(unsigned t=1136;t<1650;t+=34)h->poll(t,{},103,1,0);}
        if(fault>=9) {
            NativeRequestSample raw{};raw.valid=true;raw.state=103;raw.crouch_mode=fault==11?1u:0u;
            h->producer.observe(1125,h->owner,raw,{true,fault==9?3:1,fault==10?24u:16u});
            raw.crouch_mode=0;h->producer.observe(1130,h->owner,raw,{true,1,16});
        }
        const auto now=fault==7?1680u:fault==8?1300u:1153u;
        h->poll(now,{},200,1,0);h->poll(now+34,{},201,1,2);h->poll(now+68,{},201,-1,2);
        if(h->starts!=1 || submissions)std::cerr<<"early fault="<<fault<<" starts="<<h->starts<<'\n';
        check(h->starts==1 && submissions==0,"early intent dies on owner/OFF/authority/bail/intervening input/other action/reversal/deadline/producer gap/hidden action-object-mode excursion");
    }
    for(bool observed_jump:{false,true}) {
        auto h=std::make_unique<Route>();h->poll(1000,{},100,-1,1);
        h->poll(1017,{0x1000},100,-1,1);h->poll(1034,{0x4000},100,-1,1);h->poll(1051,{},100,-1,1);
        h->poll(1068,{},103,observed_jump?1:-1,0);h->poll(1102,{},200,observed_jump?1:-1,0);
        h->poll(1136,{},201,observed_jump?1:-1,2);h->poll(1170,{},201,-1,2);
        check(h->starts==(observed_jump?2u:1u),"a fast A/X release can await actual native Ollie confirmation but cannot borrow an unrelated fall");
    }
}
void delivered_pop_handoff() {
    for(unsigned cadence:{8u,17u,34u}) {
        auto h=std::make_unique<Route>();h->poll(1000,{},100,-1,1);
        h->poll(1017,{0x1000},100,-1,1);h->poll(1034,{0x1000},103,1,0);
        // Release delivers the actual last nonneutral pop arc. A valid AIR
        // frame alone must not truncate an undelivered arc or held setup.
        const auto pop=h->poll(1100,{0x4000},103,1,0);
        check(pop.authored_playback && std::abs(pop.right_x)==28000 && pop.right_y==28000,
            "physical release delivers the native pop before pending X takes ownership");
        check(!h->poll(1100,{0x4000},200,1,0).requested_id,
            "a repeated same-tick AIR read cannot pretend the pop was consumed");
        unsigned now=1100+cadence;
        while(now/17==1100/17) {check(!h->poll(now,{0x4000},200,1,0).requested_id,"no handoff inside the pop delivery tick");now+=cadence;}
        const auto flip=h->poll(now,{0x4000},200,1,0);
        check(flip.requested_id==2 && flip.authored_playback && !flip.right_x && !flip.right_y,
            "first distinct AIR tick hands delivered pop directly to the flip neutral reset, before old tail");
        const auto* d=h->input.input_diagnostic(0);
        check(d->dispatch[0].request.physical_pressed_at==1100,
            "pop handoff retains original physical X identity");
        h->poll(now+cadence,{},201,1,2);
        check(h->starts==2,"release does not duplicate the Ollie and single flip");
    }
}
void early_press_cadences() {
    for(unsigned rate:{120u,60u,30u}) for(unsigned delay:{0u,17u,33u,66u,100u,250u,500u,650u}) {
        auto h=std::make_unique<Route>();h->exact_clock=true;h->poll(1000,{},100,-1,1);
        h->poll(1017,{0x1000},100,-1,1);h->poll(1034,{0x1000},100,1,0);
        const unsigned release=1100;unsigned starts{}, first_motion{}, first_flick{}, recognized_at{};std::uint64_t pressed{},physical_at{};
        for(unsigned sample=0;;++sample) {
            const auto elapsed=(sample*1000+rate-1)/rate;
            if(elapsed>=delay+350)break;
            const bool held=elapsed>=delay && elapsed<delay+120;
            if(held && !physical_at)physical_at=release+elapsed;
            const auto out=h->poll(release+elapsed,{static_cast<std::uint16_t>(held?0x4000:0)},
                elapsed<40?103u:201u,elapsed<delay+250?1:-1,elapsed<40?0u:2u);
            if(out.requested_id==2){++starts;check(h->input.authored_air_followup(release+elapsed,h->frame)==2,"actual facade publishes only its admitted same-flight basic flip identity");}
            if(starts && out.authored_playback && (out.right_x || out.right_y) && !first_motion)first_motion=elapsed;
            if(starts && out.authored_playback && out.right_x && !first_flick)first_flick=elapsed;
            const auto* d=h->input.input_diagnostic(0);
            for(std::size_t i=0;i<d->recognition_count;++i)if(d->recognition[i].request.action==InputAction::flip)
                {pressed=d->recognition[i].request.physical_pressed_at;recognized_at=static_cast<unsigned>(d->recognition[i].request.recognized_at);}
        }
        check(recognized_at==pressed,"selection occurs on physical X press, independently of the pop actor");
        std::cout<<"flip_timing hz="<<rate<<" delay="<<delay<<" recognition="<<(recognized_at-pressed)<<" load="<<(release+first_motion-pressed)<<" flick="<<(release+first_flick-pressed)<<" ms\n";
        check(first_flick && first_flick<delay+250,"actual kickflip flick arc, not just load position, is delivered promptly after the physical press");
        check(starts==1 && first_motion && first_motion<delay+200,"early X emits a nonneutral native flip gesture before Ollie selection retirement at each cadence");
        check(pressed==physical_at,"handoff preserves actual sampled physical X time");
    }
}

}
void native_transition_requests() {
    struct Sink {
        unsigned calls{};std::int32_t selected{};
        static bool submit(void* self,std::int32_t id) noexcept {
            auto& s=*static_cast<Sink*>(self);++s.calls;s.selected=id;return true;
        }
    };
    struct Choice {std::uint16_t direction;int id;};
    for(bool delayed_selection:{false,true})
    for(bool retired_selection:{false,true})
    for(unsigned delay:{0u,100u,500u}) for(auto choice:{Choice{0,2},Choice{2,4},Choice{8,3},Choice{5,9}}) {
        auto h=std::make_unique<Route>();Sink sink;h->normal_flip={&sink,Sink::submit};
        h->poll(1000,{},100,-1,1);h->poll(1020,{0x1000},100,-1,1);
        h->poll(1060,{0x1000},100,delayed_selection?-1:1,delayed_selection?1:0);
        constexpr unsigned release=1100;unsigned issued{};
        for(unsigned elapsed=0;elapsed<=800;elapsed+=20) {
            const bool held=elapsed>=delay && elapsed<delay+80;
            const auto out=h->poll(release+elapsed,{static_cast<std::uint16_t>((held?0x4000u:0u)|choice.direction)},
                elapsed<120?100u:elapsed<220?103u:elapsed<260?200u:201u,
                (delayed_selection && elapsed==0) || (retired_selection && elapsed>=400)?-1:1,delayed_selection && elapsed==0?1u:elapsed<240?0u:2u);
            if(out.native_flip_request) {
                ++issued;check(out.native_flip_request==choice.id && !out.authored_playback,
                    "owned A-to-X submits its selected native transition without a second RS gesture");
                check(h->input.authored_air_followup(release+elapsed,h->frame)==choice.id,
                    "normal native directional handoff retains the original launch identity");
            }
            check(!(out.buttons&0x6000u),"X/B native push and brake remain masked during transition");
        }
        check(issued==1 && sink.calls==1 && sink.selected==choice.id,
            "A-to-X at release,100ms,500ms queues exactly one normal native selection");
    }
}
int main() {native_entry_tests::run();native_transition_requests();delivered_pop_handoff();early_press_cadences();neutral_once();directional_precedence();native_wait_and_ollie();cancellation();release_order();queued_withdrawal();prompt_directional_tap();prompt_released_ollie_tap();std::cout<<checks<<" neutral flip release production checks passed\n";}
