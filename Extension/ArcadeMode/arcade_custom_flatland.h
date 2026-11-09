#pragma once
#include "arcade_input_dispatch.h"
#include "arcade_ground_motion_step.h"
#include "arcade_movement.h"
#include "Extension/AnimationStudio/anim_core.h"
#include <memory>

namespace dingosdk::arcade_mode {
// Presentation only. No native trick ID, contact, balance or score is created.
struct CustomTrickClips {
    std::shared_ptr<const anim_studio::Clip> entry_hold, flip_exit;
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
// An explicit contact-proof boundary, with NO native producer in this spike.
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
    bool valid() const noexcept {
        return enabled && manual_admitted && balance_active && !competing_owner && !ollie &&
            manual_episode && frame.valid && InputNativeContext::from_frame(frame).fresh(now) &&
            frame.state==100 && frame.mode==1 && frame.contacts==0x4000u &&
            !(frame.stance_and_push&0x40u) && !frame.airborne && !frame.rail_family;
    }
};
class CustomFlatland {
public:
    enum class Phase { manual, entry, hold, flip_exit };
    struct Motion { float x{}, z{}; bool admitted{}; };
    CustomFlatland(CustomTrickClips clips,CustomTrickPlayback playback)
        : clips_(std::move(clips)),playback_(playback) {}
    CustomFlatland(const CustomFlatland&)=delete;
    CustomFlatland& operator=(const CustomFlatland&)=delete;
    ~CustomFlatland(){cancel();}
    Phase phase() const noexcept {return phase_;}
    bool active() const noexcept {return phase_!=Phase::manual;}
    unsigned branch() const noexcept {return phase_==Phase::flip_exit?42u:active()?2u:0u;}
    std::uint64_t episode() const noexcept {return episode_;}
    bool branch_ready() const noexcept {return phase_==Phase::manual || phase_==Phase::hold;}
    void cancel() noexcept {
        if(ticket_ && playback_.cancel)playback_.cancel(playback_.context,ticket_);
        ticket_=episode_=started_step_=0;phase_=Phase::manual;elapsed_=0;resistance_.reset();
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
        if(playback_.seek && phase_!=Phase::flip_exit)elapsed_=std::min(elapsed_,.3f);
        if(elapsed_>=duration_) {cancel();return {};}
        if(phase_==Phase::entry && elapsed_>=.3f)phase_=Phase::hold;
        if(playback_.seek)playback_.seek(playback_.context,ticket_,elapsed_);
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
        if(back){cancel();return InputDisposition::started;}
        if(!entry && !flip)return InputDisposition::unsupported;
        const auto& clip=flip?clips_.flip_exit:clips_.entry_hold;
        if(!clip || clip->empty() || !std::isfinite(clip->duration()) || clip->duration()<=0)
            return InputDisposition::unsupported;
        // A finite next phase borrows only an idle player. No ordinary play()
        // call can steal another caller's newer clip during cancellation.
        cancel();
        ticket_=playback_.begin(playback_.context,clip);
        if(!ticket_)return InputDisposition::unsupported;
        owner_=context_.frame.owner;owner_generation_=context_.frame.owner_generation;
        episode_=context_.manual_episode;started_step_=context_.frame.physics_step;
        duration_=clip->duration();phase_=flip?Phase::flip_exit:Phase::entry;
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
    float elapsed_{},duration_{};
};
}
