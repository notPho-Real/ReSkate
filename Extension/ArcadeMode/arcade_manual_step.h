#pragma once
#include "arcade_manual_hop.h"
#include "arcade_manual_release.h"
#include "arcade_balance_input.h"
#include <array>

namespace dingosdk::arcade_mode {
// Serialized with the existing native-input mutex. Each record belongs to the
// original physics observer, including its dt and physical input's source age.
// Client reads can drain records, never create intervals or renew their clocks.
class ManualPhysicsSteps {
public:
    static constexpr std::size_t capacity = 256;
    struct Interval { NativeInputFrame frame{}; float seconds{}; InputDigitalState controls{}; };
    void reset() noexcept { count_=0; fault_=false; cached_={}; seconds_=0; controls_.reset_manual(); }
    float seconds() const noexcept { return seconds_; }
    void invalidate() noexcept { count_=0; fault_=true; completion_={}; }
    NativeInputFrame completion() const noexcept { return completion_; }
    // Current native identity may outlive one missing dynamics row. This does
    // not integrate time or renew the last committed interval. Longer client
    // delays require every intervening original producer row, within 1s.
    bool continuity(const NativeInputFrame& current,std::uint64_t now) const noexcept {
        if(fault_ || !cached_.valid || !cached_.active || !current.valid ||
            now<current.sampled_at || now-current.sampled_at>150 ||
            current.sampled_at<committed_.sampled_at || current.sampled_at-committed_.sampled_at>1000 ||
            current.physics_step<consumed_ || !stable_contact(committed_,current))return false;
        if(current.physics_step==consumed_)return same_frame(committed_,current);
        if(current.sampled_at-committed_.sampled_at<=150)return true;
        auto previous=consumed_;
        for(std::size_t i=0;i<count_;++i){const auto& row=records_[i];
            if(row.frame.physics_step<=consumed_)continue;
            if(row.frame.physics_step>current.physics_step)break;
            if(row.frame.physics_step!=previous+1 || !stable_contact(committed_,row.frame))return false;
            previous=row.frame.physics_step;
        }
        return previous==current.physics_step;
    }

    void observe(const NativeInputFrame& frame,float seconds,InputDigitalState controls) noexcept {
        if(observed_ && frame.physics_step==observed_ && same_frame(frame,last_observed_.frame) &&
            seconds==last_observed_.seconds && controls.sampled_at==last_observed_.controls.sampled_at &&
            controls.buttons==last_observed_.controls.buttons && controls.stick_buttons==last_observed_.controls.stick_buttons &&
            controls.pad_buttons==last_observed_.controls.pad_buttons && controls.triggers==last_observed_.controls.triggers &&
            controls.direction==last_observed_.controls.direction && controls.available==last_observed_.controls.available)return;
        if (!frame.valid || !frame.owner.valid() || !frame.owner_generation || !frame.physics_step ||
            !std::isfinite(seconds) || seconds<.001f || seconds>.1f ||
            (observed_ && frame.physics_step<=observed_)) { invalidate(); return; }
        observed_=frame.physics_step; last_observed_={frame,seconds,controls};
        if (ManualRelease::completed_contact(frame)) completion_=frame;
        if (count_==capacity) { invalidate(); return; }
        records_[count_++]={frame,seconds,controls};
    }
    PhysicalBalanceSample sample(ManualPhysics& physics,const NativeInputFrame& current,
            SkatePose pose,std::uint64_t now,bool valid,bool preserve_history=false) noexcept {
        seconds_=0;
        const bool continuous=continuity(current,now);
        const auto retain=[&]() noexcept { return cached_; };
        const auto fail=[&]() noexcept { physics.reset(); reset(); return PhysicalBalanceSample{}; };
        if (!valid || !current.valid || !current.owner.valid() || !current.owner_generation || !current.physics_step ||
            now<current.sampled_at || now-current.sampled_at>150 || fault_) return fail();
        const bool manual=balance_kind(pose)==BalanceKind::manual;
        if (!manual) {
            // Contact and history are independent producers. A client can see
            // the matching physical contact before its unused history edge,
            // then admit that SAME original frame on a later client callback.
            // Keep only that frame's unconsumed interval; ordinary riding,
            // exits and unknown/bail contexts cannot retain a candidate.
            std::size_t keep{};
            if (pose==SkatePose::riding && current.physics_step>consumed_ && current.state==100 && current.mode==1 &&
                !(current.stance_and_push&0x40u) && (current.contacts==0x4000u || current.contacts==0x8000u)) {
                for(std::size_t i=0;i<count_;++i)if(same_frame(records_[i].frame,current)) {
                    records_[0]=records_[i]; keep=1; break;
                }
            }
            count_=keep; cached_={}; controls_.reset_manual();
            return manual_physics_pose(physics,pose,0,{},preserve_history);
        }
        if (consumed_ && current.physics_step<consumed_) return fail();
        if (current.physics_step==consumed_) {
            if (current.owner!=owner_ || current.owner_generation!=generation_ || pose!=pose_ ||
                !same_frame(current,committed_)) return fail();
            return cached_; // Includes terminal output; no force or RNG is replayed.
        }
        if(cached_.valid && (owner_!=current.owner || generation_!=current.owner_generation || pose_!=pose ||
            committed_.action_generation!=current.action_generation))return fail();
        const bool continuing=physics.active() && cached_.valid && owner_==current.owner &&
            generation_==current.owner_generation && pose_==pose;
        // First admission is tied to the current independently qualified frame.
        // Earlier records cannot retroactively acquire Manual identity.
        std::size_t first=count_;
        for(std::size_t i=0;i<count_;++i)
            if(records_[i].frame.physics_step>(continuing?consumed_:current.physics_step-1)){first=i;break;}
        if(first==count_) return continuous ? retain() : fail();
        std::uint64_t previous=continuing?consumed_:current.physics_step-1;
        auto contact_at=continuing?contact_at_:current.sampled_at;
        auto previous_frame=committed_;
        const auto wanted=pose==SkatePose::manual?0x4000u:0x8000u;
        // Validate the complete batch before advancing dynamics. Missing steps
        // cannot be reconstructed from cached dt or client elapsed time.
        for(std::size_t i=first;i<count_;++i){
            const auto& f=records_[i].frame;
            if(f.physics_step>current.physics_step)break;
            // A missed balance record may not erase a contact whose native
            // generations prove no intervening exit. Never integrate the missing dt.
            const bool stable_gap=continuing && f.physics_step>previous+1 &&
                f.contact_generation==previous_frame.contact_generation && f.contacts==wanted &&
                previous_frame.contacts==wanted && f.action_generation==previous_frame.action_generation &&
                f.sampled_at>=previous_frame.sampled_at && f.sampled_at-previous_frame.sampled_at<=150;
            if((f.physics_step!=previous+1 && !stable_gap) || f.owner!=current.owner || f.owner_generation!=current.owner_generation ||
                f.action_generation!=current.action_generation || f.state!=100 || f.mode!=1 || (f.stance_and_push&0x40u) ||
                f.sampled_at>now || (!continuous && now-f.sampled_at>150) || (continuing && f.sampled_at<sampled_at_))return fail();
            if(f.contacts==wanted)contact_at=f.sampled_at;
            else if(f.contacts!=0xc000u || f.sampled_at<contact_at || f.sampled_at-contact_at>=100)return fail();
            if(f.physics_step==current.physics_step && !same_frame(f,current))return fail();
            previous=f.physics_step; previous_frame=f;
        }
        if(previous!=current.physics_step)return continuous ? retain() : fail();
        if(!continuing)controls_.reset_manual();
        auto configured=InputBalanceControls::vertical(pose==SkatePose::nose_manual?InputBalanceKind::nose:InputBalanceKind::manual);
        configured.admitted=true; // Query mapping only; admission was supplied by the native mechanic owner.
        std::size_t taken{};
        for(std::size_t i=0;i<count_;++i){
            const auto& row=records_[i];if(row.frame.physics_step>current.physics_step)break;taken=i+1;
            if(i<first)continue;
            const auto& input=row.controls;
            const bool available=input.available && input.sampled_at<=row.frame.sampled_at && row.frame.sampled_at-input.sampled_at<=150;
            const auto pair=input.configured(configured);
            controls_.begin_manual(row.frame.sampled_at);
            controls_.poll_manual(row.frame.sampled_at,cached_.position,{pair.first,pair.second},available);
            cached_=manual_physics_pose(physics,pose,row.seconds,controls_.manual_input()); seconds_+=row.seconds;
            // An endpoint owns termination at its original interval. Later
            // buffered steps cannot restart or continue that closed section.
            if(!cached_.active)break;
        }
        for(std::size_t i=taken;i<count_;++i)records_[i-taken]=records_[i];
        count_-=taken; consumed_=current.physics_step; sampled_at_=current.sampled_at;
        owner_=current.owner;generation_=current.owner_generation;pose_=pose;contact_at_=contact_at;committed_=current;
        return cached_;
    }
    // Authoritative episode path. Unknown intervals suspend dynamics only.
    // Each accepted row is an original native dt; client time is never integrated.
    PhysicalBalanceSample sample_episode(ManualPhysics& physics,const NativeInputFrame& current,
            SkatePose pose,std::uint64_t now,bool integrate,std::uint64_t admitted_step) noexcept {
        seconds_=0;
        if(!integrate || !current.valid || now<current.sampled_at || now-current.sampled_at>150){
            count_=0; fault_=false; return {};
        }
        if(!physics.active() && !physics.closed())physics.begin(pose==SkatePose::nose_manual);
        physics.transfer(pose==SkatePose::nose_manual);
        if(fault_){count_=0;fault_=false;return {};}
        if(current.physics_step==consumed_)return physics.snapshot();
        const auto wanted=pose==SkatePose::manual?0x4000u:0x8000u;
        PhysicalBalanceSample result=physics.snapshot();
        std::size_t taken{};
        for(std::size_t i=0;i<count_;++i){
            const auto& row=records_[i];const auto& f=row.frame;
            if(f.physics_step>current.physics_step)break;
            taken=i+1;
            if(f.physics_step<=consumed_)continue;
            consumed_=f.physics_step;
            const bool known=f.physics_step>=admitted_step && f.valid && f.owner==current.owner && f.owner_generation==current.owner_generation &&
                f.state==100 && f.mode==1 && f.contacts==wanted && !(f.stance_and_push&0x40u) &&
                f.sampled_at<=now && now-f.sampled_at<=1000 &&
                row.controls.available && row.controls.sampled_at<=f.sampled_at && f.sampled_at-row.controls.sampled_at<=150;
            if(!known)continue;
            auto mapping=InputBalanceControls::vertical(pose==SkatePose::nose_manual?InputBalanceKind::nose:InputBalanceKind::manual);
            mapping.admitted=true;const auto pair=row.controls.configured(mapping);
            controls_.begin_manual(f.sampled_at);
            controls_.poll_manual(f.sampled_at,result.position,{pair.first,pair.second},true);
            result=manual_physics_pose(physics,pose,row.seconds,controls_.manual_input());
            seconds_+=row.seconds;
            if(!result.active)break;
        }
        for(std::size_t i=taken;i<count_;++i)records_[i-taken]=records_[i];
        count_-=taken;
        cached_=result;committed_=current;owner_=current.owner;generation_=current.owner_generation;pose_=pose;
        return result;
    }
private:
    static bool stable_contact(const NativeInputFrame& a,const NativeInputFrame& b) noexcept {
        return a.valid && b.valid && a.owner==b.owner && a.owner_generation==b.owner_generation &&
            a.contact_generation==b.contact_generation && a.action_generation==b.action_generation &&
            a.mode_generation==b.mode_generation && a.state==100 && b.state==100 && a.mode==1 && b.mode==1 &&
            (a.contacts==0x4000u || a.contacts==0x8000u) && a.contacts==b.contacts &&
            !(b.stance_and_push&0x40u) && a.stance_and_push==b.stance_and_push &&
            a.flip_known==b.flip_known && a.flip_id==b.flip_id && a.selection_object==b.selection_object;
    }
    static bool same_frame(const NativeInputFrame& a,const NativeInputFrame& b) noexcept {
        return a.valid==b.valid && a.owner==b.owner && a.owner_generation==b.owner_generation &&
            a.contact_generation==b.contact_generation && a.action_generation==b.action_generation &&
            a.physics_step==b.physics_step && a.sampled_at==b.sampled_at && a.state==b.state && a.mode==b.mode &&
            a.contacts==b.contacts && a.stance_and_push==b.stance_and_push && a.flip_known==b.flip_known &&
            a.flip_id==b.flip_id && a.selection_object==b.selection_object;
    }
    NativeInputFrame committed_{}, completion_{};
    Interval last_observed_{};
    std::array<Interval,capacity> records_{};
    std::size_t count_{};
    bool fault_{};
    float seconds_{};
    std::uint64_t observed_{},consumed_{},sampled_at_{},generation_{},contact_at_{};
    NativeInputOwner owner_{};
    SkatePose pose_{SkatePose::unknown};
    PhysicalBalanceSample cached_{};
    BalanceControl controls_{};
};
}
