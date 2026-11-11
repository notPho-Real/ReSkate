#pragma once
#include "arcade_input_dispatch.h"
#include "arcade_ground_motion_step.h"
#include "arcade_movement.h"
#include "Extension/AnimationStudio/motion_authoring.h"
#include <memory>

namespace dingosdk::arcade_mode {
// Presentation only. No native trick ID, contact, balance or score is created.
struct CustomTrickClips {
    std::shared_ptr<const anim_studio::Clip> entry_hold, flip_exit, direct_exit;
    float range_begin{},range_end{};
    anim_studio::motion_authoring::RangeMotion range_motion{};
};
struct CustomTrickPlayback {
    void* context{};
    std::uint64_t (*begin)(void*,std::shared_ptr<const anim_studio::Clip>){};
    bool (*owns)(void*,std::uint64_t){};
    void (*cancel)(void*,std::uint64_t){};
    void (*seek)(void*,std::uint64_t,float){}; // Optional native-time player; holds entry at 0.3 s.
    bool available() const noexcept {return begin && owns && cancel;}
};
enum class FlatlandSurface { unknown, ordinary, rough, sealed, slick };
// Joined from the original board contact and live material grid, or unknown.
// A ray/body ID or render material cannot populate this evidence.
struct FlatlandSurfaceEvidence {
    FlatlandSurface surface{};
    NativeInputOwner owner{};
    std::uint64_t owner_generation{}, contact_generation{}, physics_step{};
    bool verified_support_material{};
    bool matches(const NativeInputFrame& f) const noexcept {
        return verified_support_material && f.valid && owner==f.owner &&
            owner_generation==f.owner_generation && contact_generation==f.contact_generation &&
            physics_step==f.physics_step;
    }
};
inline GroundFriction flatland_friction(const NativeInputFrame& frame,
        const FlatlandSurfaceEvidence& surface={}) noexcept {
    float deceleration=6; // m/s²; provisional design tuning, not an engine coefficient.
    if(surface.matches(frame))switch(surface.surface) {
        case FlatlandSurface::rough: deceleration=8;break;
        case FlatlandSurface::sealed: deceleration=3;break;
        case FlatlandSurface::slick: deceleration=.75f;break;
        default: break;
    }
    return {true,false,deceleration/(60*GameplayProfile::metres_per_inch)};
}
struct CustomFlatlandContext {
    NativeInputFrame frame{};
    std::uint64_t now{}, manual_episode{};
    bool enabled{}, manual_admitted{}, balance_active{}, competing_owner{}, ollie{};
    int spin_direction{}; // Held display turn only: -1, 0, +1; physics placement stays native.
    float balance{}; // Read-only governing Manual balance; no new dynamics.
    bool valid() const noexcept {
        return std::isfinite(balance) && enabled && manual_admitted && balance_active && !competing_owner && !ollie &&
            manual_episode && frame.valid && InputNativeContext::from_frame(frame).fresh(now) &&
            frame.state==100 && frame.mode==1 && frame.contacts==0x4000u &&
            !(frame.stance_and_push&0x40u) && !frame.airborne && !frame.rail_family;
    }
};
class CustomFlatland {
public:
    enum class Phase { manual, entry, hold, flip_exit, direct_exit };
    struct Motion { float x{}, z{}; bool admitted{}; };
    CustomFlatland(CustomTrickClips clips,CustomTrickPlayback playback)
        : clips_(std::move(clips)),playback_(playback) {}
    CustomFlatland(const CustomFlatland&)=delete;
    CustomFlatland& operator=(const CustomFlatland&)=delete;
    ~CustomFlatland(){cancel();}
    Phase phase() const noexcept {return phase_;}
    bool active() const noexcept {return phase_!=Phase::manual;}
    unsigned branch() const noexcept {return phase_==Phase::flip_exit?42u:phase_==Phase::direct_exit?0u:active()?2u:0u;}
    std::uint64_t episode() const noexcept {return episode_;}
    float display_radians() const noexcept {return display_radians_;}
    bool branch_ready() const noexcept {return phase_==Phase::manual || phase_==Phase::hold;}
    void cancel() noexcept {
        if(ticket_ && playback_.cancel)playback_.cancel(playback_.context,ticket_);
        ticket_=episode_=started_step_=0;phase_=Phase::manual;elapsed_=sample_time_=range_weight_=motion_time_=display_radians_=0;resistance_.reset();
        // steps_ intentionally survives cancellation and OFF/ON.
    }
    void observe(CustomFlatlandContext context) noexcept {
        context_=context;
        if(!active())return;
        if(!context.valid() || context.manual_episode!=episode_ || context.frame.owner!=owner_ ||
            context.frame.owner_generation!=owner_generation_ ||
            context.frame.physics_step<started_step_ ||
            !playback_.owns(playback_.context,ticket_))cancel();
    }
    // Refresh proportions and the incoming pose only before the first native
    // step is submitted. Keep the accepted gesture, owner and playback lease.
    bool refresh_pending_entry(CustomTrickClips clips) noexcept {
        const auto& entry=clips.entry_hold;
        if(phase_!=Phase::entry || elapsed_!=0 || sample_time_!=0 || !context_.valid() ||
            !playback_.owns(playback_.context,ticket_) || !entry || entry->empty() ||
            !std::isfinite(entry->duration()) || entry->duration()<=0)return false;
        duration_=entry->duration();clips_=std::move(clips);return true;
    }
    InputAdapter adapter() noexcept {return {this,receive};}
    // Called only within the existing native writer transaction. The portable
    // harness supplies original-step observations at the same boundary.
    Motion step(CustomFlatlandContext context,const std::array<float,3>& velocity,
            const FlatlandSurfaceEvidence& surface={}) noexcept {
        observe(context);
        if(!active())return {};
        for(float v:velocity)if(!std::isfinite(v) || std::abs(v)>=1000){cancel();return {};}
        if(std::abs(velocity[1])>=.3f){cancel();return {};}
        if(context.frame.physics_step<=started_step_)return {};
        const auto step=steps_.consume(context.frame,context.now,true);
        if(!step) {
            if(step.status==GroundStepStatus::unavailable){cancel();}
            return {};
        }
        if(step.baseline)resistance_.reset();
        elapsed_+=step.seconds;
        if(playback_.seek && (phase_==Phase::entry || phase_==Phase::hold))elapsed_=std::min(elapsed_,.3f);
        if(elapsed_>=duration_) {cancel();return {};}
        if(phase_==Phase::entry && elapsed_>=.3f)phase_=Phase::hold;
        // Original ManualLink turns the display 360 degrees in .75 seconds.
        // Holding continues that rate; opposing triggers or release stop it.
        if(phase_==Phase::hold && (context.spin_direction==-1 || context.spin_direction==1))
            display_radians_=std::remainder(display_radians_+
                static_cast<float>(context.spin_direction)*(6.283185307179586f/.75f)*step.seconds,6.283185307179586f);
        sample_time_=elapsed_;
        if(phase_==Phase::hold && clips_.range_end>clips_.range_begin) {
            range_weight_=std::min(1.f,range_weight_+step.seconds/.10f);
            motion_time_=std::fmod(motion_time_+step.seconds,anim_studio::motion_authoring::RangeMotion::period);
            const float animated=clips_.range_motion.sample(context.balance,motion_time_);
            const float alpha=(animated*range_weight_+1)*.5f;
            sample_time_=clips_.range_begin+(clips_.range_end-clips_.range_begin)*alpha;
        }
        if(playback_.seek)playback_.seek(playback_.context,ticket_,sample_time_);
        if(phase_==Phase::direct_exit)return {}; // Direct return ends custom drag immediately.
        const float speed=std::hypot(velocity[0],velocity[2]);
        const float change=resistance_.step(step.seconds,speed,false,false,true,
            flatland_friction(context.frame,surface));
        if(change>=0 || speed==0)return {0,0,true};
        return {velocity[0]/speed*change,velocity[2]/speed*change,true};
    }
private:
    static InputDisposition receive(void* state,const InputRequest& request) noexcept {
        return static_cast<CustomFlatland*>(state)->request(request);
    }
    InputDisposition request(const InputRequest& r) noexcept {
        if(!context_.valid() || !playback_.available() || !branch_ready() ||
            r.context!=InputContext::balance || r.balance_origin!=branch() ||
            !InputNativeContext::from_frame(context_.frame).correlates_action(r.generations))
            return InputDisposition::cancelled;
        const bool entry=r.action==InputAction::balance_branch_one && r.variant==2 && !active();
        const bool flip=r.action==InputAction::balance_variant && r.variant==42 && phase_==Phase::hold;
        const bool back=r.action==InputAction::balance_return && r.variant==0 && phase_==Phase::hold;
        if(back && !clips_.direct_exit){cancel();return InputDisposition::started;}
        if(!entry && !flip && !back)return InputDisposition::unsupported;
        auto clip=back?clips_.direct_exit:flip?clips_.flip_exit:clips_.entry_hold;
        if(!clip || clip->empty() || !std::isfinite(clip->duration()) || clip->duration()<=0)
            return InputDisposition::unsupported;
        // A finite next phase borrows only an idle player. No ordinary play()
        // call can steal another caller's newer clip during cancellation.
        if(!entry) {
            try {
                clip=std::make_shared<anim_studio::Clip>(anim_studio::motion_authoring::joined(
                    *clip,*clips_.entry_hold,sample_time_));
            } catch(...) {return InputDisposition::unsupported;}
        }
        const float turn=entry?0:display_radians_;
        cancel();
        ticket_=playback_.begin(playback_.context,clip);
        if(!ticket_)return InputDisposition::unsupported;
        display_radians_=turn;
        owner_=context_.frame.owner;owner_generation_=context_.frame.owner_generation;
        episode_=context_.manual_episode;started_step_=context_.frame.physics_step;
        duration_=clip->duration();phase_=back?Phase::direct_exit:flip?Phase::flip_exit:Phase::entry;
        return InputDisposition::started;
    }
    CustomTrickClips clips_;
    CustomTrickPlayback playback_;
    CustomFlatlandContext context_{};
    NativeInputOwner owner_{};
    std::uint64_t owner_generation_{},episode_{},ticket_{},started_step_{};
    GroundMotionStep steps_;
    GroundResistance resistance_;
    Phase phase_{Phase::manual};
    float elapsed_{},sample_time_{},range_weight_{},motion_time_{},duration_{},display_radians_{};
};
}
