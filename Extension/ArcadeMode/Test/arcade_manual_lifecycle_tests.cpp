#include "Extension/ArcadeMode/arcade_input_session.h"
#include "Extension/ArcadeMode/arcade_manual_labels.h"
#include "Extension/ArcadeMode/arcade_manual_observation.h"
#include "Extension/ArcadeMode/arcade_balance_input.h"
#include "Extension/ArcadeMode/arcade_gesture.h"
#include "Extension/ArcadeMode/arcade_manual_episode.h"
#include "Extension/Skater/skater_physics.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <type_traits>
#include <vector>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* why) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
}
struct Fixture {
    InputSession session;
    ManualHistory history;
    ManualHistoryLabels labels;
    ManualObservation identity;
    ComboSession combo{native_combo_tuning()};
    NativeGesture::Output output{};
    ManualObservationResult observation{};
    std::uint64_t last{};
    bool failed{};
    NativeGesture::Output sample(std::uint64_t now, SkatePose raw, SkatePose contact,
            std::span<const ManualHistoryRow> rows = {}, std::int16_t y = 0,
            bool feedback = false, std::uint16_t buttons = 0, bool gameplay = true) {
        labels.observe(history.observe(rows, {now, 1, 2, 3, true, true}), now);
        observation = identity.poll(raw, labels.take(now),
            {now, 1, gameplay, true, true, false, false, contact});
        const auto& state = combo.model().state();
        const float correction = feedback ? std::clamp(-4 * state.balance - 2 * state.balance_velocity, -1.0f, 1.0f) : 0;
        const auto prior = state.balance_kind;
        failed = combo.step(observation.pose, last ? static_cast<float>(now - last) / 1000 : 0, correction);
        last = now;
        if (!gameplay || failed || (prior == BalanceKind::manual && balance_kind(observation.pose) != BalanceKind::manual))
            session.cancel_manual();
        InputAdmission admission{};
        admission.gameplay = gameplay; admission.authored = true; admission.manual_safe = true;
        admission.pose = balance_kind(observation.pose) == BalanceKind::manual ? observation.pose : raw;
        admission.tick = now / 17;
#ifndef ARCADE_MANUAL_LIFECYCLE_BASELINE
        admission.manual_balance_admitted = observation.confirmed && !combo.failed_section() &&
            combo.model().state().balance_kind == BalanceKind::manual;
#endif
        output = session.poll(now, {buttons, 0, y}, 0, 0, admission);
        return output;
    }
    void prepare(bool nose) {
        sample(1000, SkatePose::riding, SkatePose::riding);
        sample(1017, SkatePose::air, SkatePose::unknown, {}, static_cast<std::int16_t>(nose ? -25000 : 25000));
        check(sample(1034, SkatePose::air, SkatePose::unknown, {}, static_cast<std::int16_t>(nose ? 25000 : -25000)).manual.owns,
              "production session recognizes the airborne pair through its shared journal");
    }
};
// Synthetic original physics observations exercise the evidence predicates;
// they do not establish that the candidate LS recipe performs a native Pivot.
void physical_pivot_evidence() {
    using Status=ManualEpisode::PivotStatus;
    constexpr float pi=3.14159265358979323846f;
    for(bool nose:{false,true})for(bool fakie:{false,true})for(unsigned hz:{30u,60u,120u})
    for(float origin:{-3.0f,-.2f,1.6f})for(int sign:{-1,1})for(unsigned fault=0;fault<31;++fault) {
        ManualEpisode episode;NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};
        NativeRequestSample raw{};raw.valid=true;raw.state=100;raw.crouch_mode=1;
        const auto contacts=nose?0x8000u:0x4000u;raw.words[1]=contacts;
        dingosdk::NativeOrientation orientation{true,fakie};
        auto frame=producer.observe(1000,owner,raw,{true,-1},false,1.f/hz,orientation);
        ManualInputDiagnostic command{};command.owns=command.safe=command.confirmed=true;
        command.request=nose?1:-1;command.request_at=1000;
        episode.delivery(frame,command,true);
        episode.admit(frame,{nose?SkatePose::nose_manual:SkatePose::manual,true,true});
        const auto serial=episode.serial();
        const auto motion=[&](float board,float body) {
            return ManualEpisode::PivotMotion{true,frame.owner,frame.owner_generation,frame.physics_step,frame.sampled_at,
                {std::sin(origin+board),0,std::cos(origin+board)},
                {std::sin(origin+.4f+body),0,std::cos(origin+.4f+body)}};
        };
        const auto initial=motion(0,0);
        auto unavailable=episode;
        check(!unavailable.begin_pivot(1000,frame,initial,1151),"expired original support cannot begin Pivot evidence");
        unavailable=episode;
        auto absent=initial;absent.valid=false;
        check(!unavailable.begin_pivot(1000,frame,absent,1000),"missing native body vectors cannot begin Pivot evidence");
        unavailable=episode;
        auto relabeled=frame;++relabeled.sampled_at;
        auto rebound=initial;rebound.sampled_at=relabeled.sampled_at;
        check(!unavailable.begin_pivot(1000,relabeled,rebound,1001),
            "a matching step number cannot rebind the original episode to a fresh pose timestamp");
        check(episode.begin_pivot(1000,frame,initial,1000),"owned supported Manual starts physical Pivot observation");
        check(!episode.begin_pivot(1000,frame,initial,1000),"one admitted transition cannot restart its physical evidence");
        NativeInputFrame turning{};
        const unsigned turn_steps=hz/2;
        for(unsigned step=1;step<=turn_steps+3;++step) {
            const bool mid=step==turn_steps/2;
            const bool returned=step>turn_steps;
            const auto at=1000+std::uint64_t{step}*1000/hz;
            raw.state=returned || fault==23?100:101;raw.words[1]=contacts;
            raw.words[0]=0;raw.crouch_mode=1;
            if(mid && fault==1)raw.words[1]=0xc000u;
            if(mid && fault==2)raw.words[1]=0;
            if(mid && fault==3)raw.words[1]=nose?0x4000u:0x8000u;
            if(mid && fault==4)++owner.rig;
            if(mid && fault==5)producer.invalidate();
            if(mid && fault==24)raw.state=200;
            if(mid && fault==25)raw.state=300;
            if(mid && fault==26)raw.state=600;
            if(mid && fault==27)raw.words[0]=0x40u;
            if(mid && fault==28)raw.crouch_mode=0;
            if(returned && fault!=6)orientation.fakie=!fakie;
            if(returned && fault==6)raw.words[0]=0x20u; // Leading foot is not proof of a physical turn.
            if(mid && fault==7)(void)producer.observe(at-1,owner,raw,{true,-1},false,1.f/hz,orientation);
            frame=producer.observe(at,owner,raw,{true,-1},false,1.f/hz,orientation);
            if(mid && fault==29)frame.valid=false;
            if(returned && fault==30)frame.orientation.valid=false;
            if(step==1)turning=frame;
            const auto transition=mid && fault==8?1001u:1000u;
            episode.observe(frame,transition);
            if(step==turn_steps+1) {
                command.revert={frame.owner_generation,turning.physics_step,turning.contact_generation,at,0};
                command.request_at=at;
                episode.delivery(frame,command,true);
            }
            const float progress=static_cast<float>(std::min(step,turn_steps))/turn_steps;
            const float angle=sign*pi*progress;
            const float board=fault==9 || fault==11?0:fault==12?angle*.5f:fault==13?angle*3:
                fault==14?(step<turn_steps/2?0:sign*pi):angle;
            const float body=fault==9 || fault==10?0:fault==12?angle*.5f:fault==13?angle*3:
                fault==15?-angle:fault==14?(step<turn_steps/2?0:sign*pi):angle;
            auto physical=motion(board,body);
            if(mid && fault==16)physical.valid=false;
            if(mid && fault==17)++physical.owner.core;
            if(mid && fault==18)++physical.physics_step;
            if(mid && fault==19)++physical.sampled_at;
            if(mid && fault==20)physical.board_forward={0,1,0};
            if(mid && fault==21)physical.body_forward[0]=std::numeric_limits<float>::quiet_NaN();
            episode.observe_pivot(frame,physical,transition);
            const auto before=episode.pivot_evidence();
            episode.observe_pivot(frame,physical,transition);
            check(episode.pivot_evidence().board_radians==before.board_radians &&
                episode.pivot_evidence().body_radians==before.body_radians,
                "duplicate client observations cannot add physical rotation or settle contact");
            if(mid && fault==22) {
                physical.board_forward={std::sin(origin+.1f),0,std::cos(origin+.1f)};
                episode.observe_pivot(frame,physical,transition);
            }
            if(step<turn_steps+3)check(episode.pivot_evidence().status!=Status::observed,
                "angle or fakie change cannot replace independently settled successor balance");
        }
        const auto& result=episode.pivot_evidence();
        if((result.status==Status::observed)!=(fault==0))
            std::cerr<<"pivot fault="<<fault<<" hz="<<hz<<" nose="<<nose<<" fakie="<<fakie
                <<" origin="<<origin<<" sign="<<sign<<" status="<<static_cast<unsigned>(result.status)<<'\n';
        check((result.status==Status::observed)==(fault==0),
            "only a continuous supported same-owner board and body half turn proves physical Pivot evidence");
        if(!fault) {
            check(result.episode==serial && result.transition==1000 && result.completed_step==frame.physics_step &&
                result.completed_at==frame.sampled_at && result.successor==(nose?SkatePose::nose_manual:SkatePose::manual) &&
                std::abs(result.board_radians-sign*pi)<.0001f && std::abs(result.body_radians-sign*pi)<.0001f,
                "physical Pivot receipt retains exact native episode, step and measured signed rotations");
        }
        if(fault!=4 && fault!=5 && fault!=8 && fault!=25 && fault!=26)check(episode.established() && episode.owned() && episode.serial()==serial,
            "unproved Pivot never cancels independently restored Manual balance or ownership");
        check(!episode.begin_pivot(1000,frame,motion(pi,pi),frame.sampled_at),
            "completed or rejected occurrence cannot restart from a later physical endpoint");
    }
}
void physical_board_reference_reader() {
    namespace state=dingosdk::game::build::v20260929::skater_state;
    namespace body=dingosdk::game::build::v20260929::skater_body;
    namespace source=dingosdk::game::build::v20260929::client_source_spawn;
    for(unsigned fault=0;fault<14;++fault) {
        const dingosdk::NoBailSkater owner{0x10000000,0x20000,0x30000,0x40000,0x50000};
        constexpr std::uintptr_t holder=0x60000,physics=0x70000,parts=0x80000;
        const auto reference=parts+source::reference_body_index*state::physics_body_size;
        std::map<std::uintptr_t,std::vector<unsigned char>> memory;
        std::map<std::uintptr_t,unsigned> reads;
        const auto put=[&](std::uintptr_t at,const auto& value) {
            auto& bytes=memory[at];bytes.resize(sizeof(value));std::memcpy(bytes.data(),&value,sizeof(value));
        };
        put(owner.core+state::board_holder_offset,holder);
        put(holder+state::board_physics_offset,physics);
        put(physics,owner.base+state::board_physics_vtable+(fault==1?8u:0u));
        put(physics+state::physics_bodies_offset,parts);
        put(parts,fault==2?state::rig_body_count:state::board_body_count);
        put(reference+state::body_owner_offset,physics+(fault==3?8u:0u));
        dingosdk::skater_physics::Step::Pose pose{};
        pose.transform={0,0,-1,0,0,1,0,0,1,0,0,0,4,5,6,1};pose.velocity={1,2,3};
        if(fault==4)pose.transform[12]=std::numeric_limits<float>::quiet_NaN();
        if(fault==5)pose.transform[0]=2;
        if(fault==6)pose.velocity[1]=std::numeric_limits<float>::infinity();
        if(fault!=7)put(reference+body::body_transform_offset,pose.transform);
        put(reference+state::body_velocity_offset,pose.velocity);
        const auto read=[&](std::uintptr_t at,auto& value) {
            const auto nth=++reads[at];
            const bool replaced=(fault==8 && nth==2 && at==owner.core+state::board_holder_offset) ||
                (fault==9 && nth==2 && at==holder+state::board_physics_offset) ||
                (fault==10 && nth==2 && at==physics) ||
                (fault==11 && nth==2 && at==physics+state::physics_bodies_offset) ||
                (fault==12 && nth==2 && at==reference+state::body_owner_offset) ||
                (fault==13 && nth==2 && at==parts);
            if constexpr(std::is_integral_v<std::remove_cvref_t<decltype(value)>>)
                if(replaced){value=0;return true;}
            const auto found=memory.find(at);
            if(found==memory.end() || found->second.size()!=sizeof(value))return false;
            std::memcpy(&value,found->second.data(),sizeof(value));return true;
        };
        const auto observed=dingosdk::skater_physics::detail::read_board_reference_pose(read,owner);
        check(observed.has_value()==(fault==0),"physical board pose rejects invalid or replaced native pool, body and matrix");
        if(observed)check(observed->transform==pose.transform && observed->velocity==pose.velocity,
            "physical board observation reads reference8 from the nine-body board pool");
        check(!reads.contains(owner.rig+state::rig_physics_offset) &&
            !reads.contains(parts+body::body_transform_offset),
            "rig forearm8 and board root0 are never substituted for the board orientation reference");
    }
}
}
int main() {
    physical_board_reference_reader();
    physical_pivot_evidence();
    for (bool nose : {false, true}) {
        Fixture f; f.prepare(nose);
        const auto wanted = nose ? SkatePose::nose_manual : SkatePose::manual;
        for (std::uint64_t now = 1051; now < 4000; now += 17) {
            f.sample(now, wanted, wanted);
            check(!balance_meter(f.combo.balance_display_state()).visible,
                  "unlabelled physical contact cannot fabricate the balance lifecycle");
        }
        check(!f.output.manual.owns && !f.output.right_y,
              "unadmitted contact cannot hold unified native actuation indefinitely");
    }
    for (bool nose : {false, true}) for (bool early : {false, true}) {
        Fixture f; f.prepare(nose);
        const auto wanted = nose ? SkatePose::nose_manual : SkatePose::manual;
        const std::array rows{ManualHistoryRow{10, 20, nose ? 2u : 1u}};
        if (early) {
            f.sample(1051, SkatePose::air, SkatePose::unknown, rows);
            check(!f.observation.confirmed && !balance_meter(f.combo.balance_display_state()).visible,
                  "an early history edge alone never activates balance");
        }
        for (std::uint64_t now = early ? 1068 : 1051; now < 61000; now += 17) {
            // Inactive generic scoring reports riding; independently validated
            // contact and a fresh history occurrence must still establish identity.
            f.sample(now, SkatePose::riding, wanted, rows, 0, true);
            check(f.observation.confirmed && !f.failed && f.output.manual.owns &&
                f.output.right_y == (nose ? 20000 : -20000) && balance_meter(f.combo.balance_display_state()).visible,
                  "legitimate history/contact ordering drives production actuation and visible corrected balance without a lifespan cap");
        }
        check(f.combo.model().state().manual_fatigue > 0,
              "admitted session owns a real correction/fatigue lifecycle");
        check(f.sample(61000, wanted, wanted, rows, 0, true, 0x2000).manual.owns,
              "ground B is grab-only and preserves admitted manual actuation");
        check(!f.sample(61017, wanted, wanted, rows, 0, true, 0, false).manual.owns,
              "gameplay loss releases the shared session");
    }
#ifndef ARCADE_MANUAL_LIFECYCLE_BASELINE
    for (bool admitted : {false, true}) {
        ArcadeInput input; input.enable(true);
        const auto sample = [&](std::uint64_t now, std::int16_t y, SkatePose pose) {
            return input.poll(0, now, 0, 0, y, 0, 0, true, false, false, 0, false,
                false, true, now / 17, false, {}, false, true, pose, false, false,
                false, false, true, false, 0, 0, ActionExperiment::off, false, false, admitted);
        };
        sample(1000, 0, SkatePose::air); sample(1017, 25000, SkatePose::air);
        check(sample(1034, -25000, SkatePose::air).manual.owns,
              "the remap facade routes the pair into the production session");
        NativeGesture::Output output{};
        for (std::uint64_t now = 1051; now < 5000; now += 17) output = sample(now, 0, SkatePose::manual);
        check(output.manual.owns == admitted && (output.right_y == -20000) == admitted,
              "the remap facade forwards independent balance authority without inferring it from pose");
        input.enable(false);
        check(!sample(5017, 0, SkatePose::manual).manual.owns, "OFF clears the production facade ownership");
    }
#endif
    std::cout << checks << " unified manual lifecycle checks passed\n";
}
