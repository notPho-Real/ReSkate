#pragma once
#include "arcade_manual_balance_entry.h"

namespace dingosdk::arcade_mode {
// Sole lifetime owner. Acquisition observers can admit an episode, never end
// one. Only original native frames advance contact/exit evidence. Consumers
// may suspend actuation, but cannot turn missing evidence into physical exit.
class ManualEpisode {
public:
    enum class Phase { inactive, acquiring, active, transitioning, airborne_suspended, combo_airborne, ended };
    enum class Exit { none, disabled, owner, offboard, grind, bail, putdown, incompatible, endpoint };
    // Physical body observations from the same original physics callback as
    // frame. Entity placement, animation/display poses and fakie flags are not
    // substitutes for these native board-reference and pelvis basis vectors.
    struct PivotMotion {
        bool valid{};
        NativeInputOwner owner{};
        std::uint64_t owner_generation{}, physics_step{}, sampled_at{};
        std::array<float,3> board_forward{}, body_forward{};
        bool operator==(const PivotMotion&) const = default;
    };
    enum class PivotStatus { inactive, tracking, rejected, observed };
    enum class PivotRejection { none, identity, motion, support, sampling, rotation, successor };
    struct PivotEvidence {
        PivotStatus status{PivotStatus::inactive};
        PivotRejection rejection{PivotRejection::none};
        std::uint64_t transition{}, episode{}, started_at{}, completed_at{}, completed_step{};
        SkatePose origin{SkatePose::unknown}, successor{SkatePose::unknown};
        float board_radians{}, body_radians{};
    };
    const PivotEvidence& pivot_evidence() const noexcept { return pivot_; }
    // Evidence only. Failure never cancels the existing Manual/revert owner,
    // balance restoration or combo, and success grants no native score credit.
    bool begin_pivot(std::uint64_t transition,const NativeInputFrame& frame,
                     const PivotMotion& motion,std::uint64_t now) noexcept {
        if(!transition || transition>now || pivot_.status==PivotStatus::tracking ||
            (pivot_.episode==serial_ && transition<=pivot_.transition))return false;
        pivot_={PivotStatus::tracking,PivotRejection::none,transition,serial_,frame.sampled_at,0,0,pose_};
        pivot_frame_=frame;pivot_motion_=motion;pivot_board_path_=pivot_body_path_=0;pivot_native_turn_=false;
        if(!owned_ || !actuatable(now) || !pivot_frame_current(frame) ||
            frame.owner!=owner_ || frame.owner_generation!=generation_ || !frame.orientation.valid) {
            reject_pivot(PivotRejection::identity);return false;
        }
        if(!pivot_support(frame)) {reject_pivot(PivotRejection::support);return false;}
        if(!pivot_motion_current(frame,motion)) {reject_pivot(PivotRejection::motion);return false;}
        pivot_initial_orientation_=frame.orientation;
        return true;
    }
    // Call after observe() on EVERY original physics step while tracking. A
    // missing read must be supplied as invalid, never silently skipped. The
    // existing episode proves successor balance; this adds actual supported
    // board/body rotation, independently of labels and requested LS movement.
    void observe_pivot(const NativeInputFrame& frame,const PivotMotion& motion,std::uint64_t transition) noexcept {
        if(pivot_.status!=PivotStatus::tracking)return;
        if(!frame.valid || !established() || !owned_ || uncertain_ || release_step_ ||
            serial_!=pivot_.episode || transition!=pivot_.transition ||
            frame.owner!=pivot_frame_.owner || frame.owner_generation!=pivot_frame_.owner_generation ||
            !pivot_frame_current(frame) || frame.owner!=owner_ || frame.owner_generation!=generation_) {
            reject_pivot(PivotRejection::identity);return;
        }
        if(!pivot_support(frame)) {reject_pivot(PivotRejection::support);return;}
        if(!pivot_motion_current(frame,motion)) {reject_pivot(PivotRejection::motion);return;}
        if(frame.physics_step==pivot_frame_.physics_step) {
            if(motion!=pivot_motion_ || frame.state!=pivot_frame_.state || frame.contacts!=pivot_frame_.contacts ||
                frame.action_generation!=pivot_frame_.action_generation ||
                frame.contact_generation!=pivot_frame_.contact_generation || frame.orientation!=pivot_frame_.orientation)
                reject_pivot(PivotRejection::sampling);
            return;
        }
        if(frame.physics_step!=pivot_frame_.physics_step+1 || frame.sampled_at<=pivot_frame_.sampled_at ||
            frame.sampled_at-pivot_frame_.sampled_at>150 || frame.mode_generation!=pivot_frame_.mode_generation ||
            frame.action_generation<pivot_frame_.action_generation || frame.contact_generation<pivot_frame_.contact_generation) {
            reject_pivot(PivotRejection::sampling);return;
        }
        const auto board=pivot_delta(pivot_motion_.board_forward,motion.board_forward);
        const auto body=pivot_delta(pivot_motion_.body_forward,motion.body_forward);
        // Beyond a quarter turn between observations the direction/path is
        // ambiguous. These are evidence tolerances, never physics/input tuning.
        constexpr float pi=3.14159265358979323846f;
        if(std::abs(board)>pi*.5f || std::abs(body)>pi*.5f) {reject_pivot(PivotRejection::rotation);return;}
        pivot_.board_radians+=board;pivot_.body_radians+=body;
        pivot_board_path_+=std::abs(board);pivot_body_path_+=std::abs(body);
        pivot_native_turn_=pivot_native_turn_ || frame.state==101;
        pivot_frame_=frame;pivot_motion_=motion;
        // Reject extra revolutions and large reversals rather than accepting
        // an arbitrary odd half-turn from endpoint direction alone.
        if(pivot_board_path_>pi*4/3 || pivot_body_path_>pi*4/3) {reject_pivot(PivotRejection::rotation);return;}
        if(phase_!=Phase::active || !transition_completed_ || transition_id_!=pivot_.transition)return;
        if(!pivot_native_turn_ || !frame.orientation.valid ||
            frame.orientation.fakie==pivot_initial_orientation_.fakie ||
            frame.state!=100 || frame.mode!=1 ||
            frame.contacts!=(pose_==SkatePose::manual?0x4000u:0x8000u)) {
            reject_pivot(PivotRejection::successor);return;
        }
        constexpr float tolerance=pi/6; // Both measured bodies must turn180 +/-30 degrees.
        if(std::abs(std::abs(pivot_.board_radians)-pi)>tolerance ||
            std::abs(std::abs(pivot_.body_radians)-pi)>tolerance ||
            pivot_.board_radians*pivot_.body_radians<=0 ||
            std::abs(pivot_.board_radians-pivot_.body_radians)>tolerance) {
            reject_pivot(PivotRejection::rotation);return;
        }
        pivot_.status=PivotStatus::observed;pivot_.successor=pose_;
        pivot_.completed_at=frame.sampled_at;pivot_.completed_step=frame.physics_step;
    }
    void reset() noexcept { phase_=Phase::inactive; exit_=Exit::none; pose_=SkatePose::unknown; last_={}; two_at_=transition_id_=successor_contact_step_=release_step_=delivery_at_=0; successor_={}; owned_=transition_completed_=resume_pending_=uncertain_=reported_=physical_exit_=false;command_={};pivot_={}; }
    Phase phase() const noexcept { return phase_; }
    Exit exit() const noexcept { return exit_; }
    std::uint64_t serial() const noexcept { return serial_; }
    std::uint64_t integration_step() const noexcept { return integration_step_; }
    bool established() const noexcept { return phase_==Phase::active || phase_==Phase::transitioning || phase_==Phase::airborne_suspended; }
    bool grounded() const noexcept { return (phase_==Phase::active || phase_==Phase::transitioning) && last_.state!=103; }
    bool transitioning() const noexcept { return phase_==Phase::transitioning; }
    bool airborne() const noexcept { return phase_==Phase::airborne_suspended || phase_==Phase::combo_airborne; }
    bool ended() const noexcept { return phase_==Phase::ended; }
    SkatePose pose() const noexcept { return airborne()?SkatePose::air:last_.state==103?SkatePose::riding:pose_; }
    bool fresh(std::uint64_t now) const noexcept { return last_.valid && now>=last_.sampled_at && now-last_.sampled_at<=150; }
    bool actuatable(std::uint64_t now) const noexcept {
        return !uncertain_ && !release_step_ && (!owned_ || (now>=delivery_at_ && now-delivery_at_<=150)) &&
            phase_==Phase::active && fresh(now) && last_.state==100 && last_.mode==1 &&
            last_.contacts==(pose_==SkatePose::manual?0x4000u:0x8000u);
    }
    bool owned() const noexcept { return owned_; }
    void suspend() noexcept { resume_pending_=owned_ && phase_==Phase::active; }
    ManualInputDiagnostic take_resume(std::uint64_t now) noexcept {
        if(!resume_pending_ || !actuatable(now))return {};
        resume_pending_=false;return command_;
    }
    NativeInputFrame termination_frame() const noexcept { return terminal_; }
    bool take_termination() noexcept { if(!ended() || reported_)return false;reported_=true;return true; }
    void end(Exit reason) noexcept { if(phase_!=Phase::inactive && phase_!=Phase::ended){phase_=Phase::ended;exit_=reason;terminal_=last_;reported_=false;successor_={};owned_=resume_pending_=false;physical_exit_=reason==Exit::putdown || reason==Exit::bail || reason==Exit::grind || reason==Exit::offboard || reason==Exit::incompatible;two_at_=0;if(pivot_.status==PivotStatus::tracking)reject_pivot(PivotRejection::identity);} }
    void release_delivery(const NativeInputFrame& f,bool physical_release) noexcept {
        if(!physical_release || !established() || !f.valid || f.owner!=owner_ ||
            f.owner_generation!=generation_ || f.state!=100 || f.mode!=1 || !f.physics_step)return;
        // A real neutral packet following physical cancellation precedes the
        // physical completion. Missing delivery or an unsafe poll is not release.
        release_step_=f.physics_step;two_at_=0;owned_=resume_pending_=false;
    }
    void delivery(const NativeInputFrame& f,const ManualInputDiagnostic& d,bool eligible) noexcept {
        if(!eligible || !f.valid || !d.owns || !d.safe || (d.request!=-1 && d.request!=1))return;
        if(uncertain_ && d.request_at>command_.request_at){phase_=Phase::acquiring;uncertain_=false;owner_=f.owner;generation_=f.owner_generation;}
        if(!established() && phase_!=Phase::combo_airborne){phase_=Phase::acquiring;exit_=Exit::none;owner_=f.owner;generation_=f.owner_generation;}
        if(owner_!=f.owner || generation_!=f.owner_generation)return;
        if(release_step_ && d.request_at<=command_.request_at)return;
        release_step_=0;owned_=true;delivery_at_=f.sampled_at;command_=d;
        if(transitioning() && d.revert.owner==generation_ && d.revert.ack_at &&
            d.revert.step>=entry_step_ && d.request==(pose_==SkatePose::manual?-1:1)) {
            successor_=d.revert; successor_step_=f.physics_step; successor_direction_=d.request;
        }
    }
    void admit(const NativeInputFrame& f,const ManualObservationResult& admission) noexcept {
        if(!ended() && uncertain_ && admission.admitted && f.owner==owner_){phase_=Phase::acquiring;uncertain_=false;}
        if(established() || !f.valid || !admission.confirmed || balance_kind(admission.pose)!=BalanceKind::manual)return;
        // Independent native history is also an admission route. A fresh label
        // may reopen a physically completed section without an Arcade RS packet;
        // a requested endpoint or a lingering label alone cannot do so.
        if(ended() && (!admission.admitted || !physical_exit_ || f.owner!=owner_ || f.owner_generation!=generation_ ||
            f.physics_step<=terminal_.physics_step || f.sampled_at<=terminal_.sampled_at))return;
        if(f.state!=100 || f.mode!=1 || (f.stance_and_push&0x40u) ||
            f.contacts!=(admission.pose==SkatePose::manual?0x4000u:0x8000u))return;
        if(serial_==UINT64_MAX)return;
        ++serial_;phase_=Phase::active;exit_=Exit::none;pose_=admission.pose;physical_exit_=uncertain_=false;
        // A newly proved native section has no relationship to the previous
        // section's neutral release packet, including independent history entry.
        release_step_=0;
        owner_=f.owner;generation_=f.owner_generation;entry_step_=integration_step_=f.physics_step;last_=f;two_at_=0;
    }
    // Return true only for a positive terminal boundary, including boundaries
    // observed between client ticks. Never infer a putdown from time alone.
    bool observe(const NativeInputFrame& f,std::uint64_t transition,bool bail=false) noexcept {
        if(!f.valid)return false;
        if(ended() && !physical_exit_){
            if(f.owner!=owner_ || f.owner_generation!=generation_ ||
                f.physics_step<=last_.physics_step || f.sampled_at<last_.sampled_at)return false;
            const bool consecutive=f.physics_step==last_.physics_step+1 && f.sampled_at-last_.sampled_at<=150;
            last_=f;
            const bool boundary=bail || f.state==300 || f.state==504 || f.rail_family || f.state==701 || f.airborne;
            if(f.state==100 && f.mode==1 && f.contacts==0xc000u){
                if(!two_at_ || !consecutive)two_at_=f.sampled_at;
            }else two_at_=0;
            if(boundary || (two_at_ && f.sampled_at-two_at_>=100)){
                physical_exit_=true;terminal_=f;
            }
            return false;
        }
        if(!established() && phase_!=Phase::combo_airborne)return false;
        const auto finish=[&](Exit reason){last_=f;end(reason);return true;};
        if(owner_!=f.owner)return finish(Exit::owner);
        if(bail || f.state==300)return finish(Exit::bail);
        if(f.state==504)return finish(Exit::offboard);
        if(f.rail_family || f.state==701)return finish(Exit::grind);
        if(generation_!=f.owner_generation){
            // Observer epochs also advance on a failed read. Same complete
            // native owner plus a short, matching return can revalidate identity.
            const bool revalidated=f.sampled_at>=last_.sampled_at && f.sampled_at-last_.sampled_at<=150 &&
                f.state==100 && f.mode==1 && f.contacts==(pose_==SkatePose::manual?0x4000u:0x8000u);
            uncertain_=!revalidated;
            generation_=f.owner_generation;resume_pending_=revalidated && owned_;
            if(uncertain_)owned_=false;
        }
        if(f.physics_step<=last_.physics_step)return false;
        const bool consecutive=f.physics_step==last_.physics_step+1 && f.sampled_at>=last_.sampled_at && f.sampled_at-last_.sampled_at<=150;
        last_=f;
        if(f.airborne){
            if(!airborne()){air_at_=f.sampled_at;phase_=Phase::airborne_suspended;}
            if(f.sampled_at>=air_at_ && f.sampled_at-air_at_>=500)phase_=Phase::combo_airborne;
            two_at_=successor_contact_step_=0;successor_={};return false;
        }
        if(phase_==Phase::combo_airborne){phase_=Phase::inactive;owned_=false;return false;}
        if(phase_==Phase::airborne_suspended && f.state==100 && f.mode==1 && f.contacts==0xc000u){
            // Takeoff cancels the old native pressure. A short flight retains
            // combo identity, not an indefinite ownership claim on landing.
            // Only a delivered preparation from this flight can retain it.
            phase_=Phase::active;
            if(delivery_at_<air_at_){owned_=false;uncertain_=false;resume_pending_=false;}
        }
        if(transition!=transition_id_){transition_id_=transition;transition_completed_=false;successor_contact_step_=0;}
        if(transition && !transition_completed_){phase_=Phase::transitioning;two_at_=0;}
        if(transitioning()){
            const bool linked=successor_.owner==generation_ && successor_.ack_at && f.sampled_at>=successor_.ack_at &&
                f.sampled_at-successor_.ack_at<=1100 && f.physics_step>successor_step_ && f.physics_step>successor_.step &&
                f.contact_generation>successor_.contact &&
                f.state==100 && f.mode==1 && f.contacts==(successor_direction_<0?0x4000u:0x8000u);
            // A first returning wheel touch is provisional: the native board
            // can rebound to both trucks while the delivered pressure settles.
            // Complete only when another consecutive ORIGINAL observation
            // confirms that contact. Client/input duplicates cannot establish
            // continuity, and a contact/read gap restarts this evidence pair.
            const bool settled=linked && consecutive && successor_contact_step_ &&
                f.physics_step==successor_contact_step_+1;
            successor_contact_step_=linked?f.physics_step:0;
            if(settled){integration_step_=f.physics_step;pose_=successor_direction_<0?SkatePose::manual:SkatePose::nose_manual;phase_=Phase::active;successor_={};successor_contact_step_=0;transition_completed_=true;return false;}
            if(transition)return false;
            // An expired link is not a physical exit. Fresh incompatible state
            // or a sustained two-truck observation below must establish it.
            phase_=Phase::active;
        }
        if(f.state==103){phase_=Phase::active;two_at_=0;return false;}
        if(f.state!=100 || f.mode!=1 || (f.stance_and_push&0x40u)){end(Exit::incompatible);return true;}
        const auto wanted=pose_==SkatePose::manual?0x4000u:0x8000u;
        if(f.contacts==wanted){phase_=Phase::active;two_at_=0;return false;}
        if(f.contacts==0xc000u){
            // Held native pressure and wheel contact are separate evidence.
            // Ground entry and established manuals can rebound onto both
            // trucks. Cancelling pressure here creates the putdown it claims
            // to observe. No balance interval is integrated on these contacts.
            if(owned_ || uncertain_){two_at_=0;return false;}
            if(release_step_ && f.physics_step<=release_step_)return false;
            if(!two_at_ || !consecutive)two_at_=f.sampled_at;
            // Unactuated native sections, or an explicitly released owned
            // section, still require consecutive original physical completion.
            if(f.sampled_at-two_at_>=100){end(Exit::putdown);return true;}
        }else {two_at_=0;if(f.contacts==0x4000u || f.contacts==0x8000u){end(Exit::incompatible);return true;}}
        return false;
    }
private:
    void reject_pivot(PivotRejection reason) noexcept {pivot_.status=PivotStatus::rejected;pivot_.rejection=reason;}
    bool pivot_frame_current(const NativeInputFrame& frame) const noexcept {
        return frame.valid && last_.valid && frame.owner==last_.owner &&
            frame.owner_generation==last_.owner_generation && frame.physics_step==last_.physics_step &&
            frame.sampled_at==last_.sampled_at && frame.contact_generation==last_.contact_generation &&
            frame.action_generation==last_.action_generation && frame.mode_generation==last_.mode_generation &&
            frame.state==last_.state && frame.mode==last_.mode && frame.contacts==last_.contacts &&
            frame.stance_and_push==last_.stance_and_push && frame.orientation==last_.orientation;
    }
    bool pivot_support(const NativeInputFrame& frame) const noexcept {
        // Reuse the admitted native wheel identity. Fakie/switch changes do
        // not swap the physically supporting truck; both-truck revert contact
        // is not evidence of a wheel-supported Pivot.
        return frame.valid && !frame.airborne && !frame.rail_family && !(frame.stance_and_push&0x40u) &&
            (frame.state==100 || frame.state==101) && frame.mode==1 &&
            frame.contacts==(pivot_.origin==SkatePose::manual?0x4000u:0x8000u);
    }
    static bool pivot_motion_current(const NativeInputFrame& frame,const PivotMotion& motion) noexcept {
        if(!motion.valid || !frame.owner.valid() || !frame.owner_generation || !frame.physics_step ||
            motion.owner!=frame.owner || motion.owner_generation!=frame.owner_generation ||
            motion.physics_step!=frame.physics_step || motion.sampled_at!=frame.sampled_at)return false;
        for(const auto& vector:{motion.board_forward,motion.body_forward}) {
            float square{};
            for(float v:vector) {if(!std::isfinite(v))return false;square+=v*v;}
            if(std::abs(square-1)> .05f || vector[0]*vector[0]+vector[2]*vector[2]<.25f)return false;
        }
        return true;
    }
    static float pivot_delta(const std::array<float,3>& before,const std::array<float,3>& after) noexcept {
        return std::atan2(before[2]*after[0]-before[0]*after[2],before[0]*after[0]+before[2]*after[2]);
    }
    PivotEvidence pivot_{};PivotMotion pivot_motion_{};NativeInputFrame pivot_frame_{};
    NativeOrientation pivot_initial_orientation_{};
    float pivot_board_path_{},pivot_body_path_{};bool pivot_native_turn_{};
    Phase phase_{Phase::inactive};Exit exit_{Exit::none};SkatePose pose_{SkatePose::unknown};
    NativeInputOwner owner_{};NativeInputFrame last_{};ManualRevertContact successor_{};
    std::uint64_t serial_{},generation_{},entry_step_{},integration_step_{},two_at_{},air_at_{},successor_step_{},transition_id_{},successor_contact_step_{},release_step_{},delivery_at_{};
    int successor_direction_{};bool owned_{},transition_completed_{},resume_pending_{},uncertain_{},reported_{},physical_exit_{};
    NativeInputFrame terminal_{};
    ManualInputDiagnostic command_{};
};
}
