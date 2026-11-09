#include "arcade_custom_flatland_runtime.h"
#if defined(DINGOSDK_DEV_TOOLS)
#include "arcade_custom_flatland_clips.h"
#include "Extension/Skater/skater_animation.h"
#include <mutex>

namespace dingosdk::arcade_mode::custom_flatland_runtime {
namespace {
struct State {
    std::mutex mutex;
    CustomFlatlandContext context;
    NativeInputOwner seed_owner{};
    std::uint64_t seed_generation{},seed_episode{},seed_step{},seed_at{};
    std::uint32_t seed_stance{};
    std::uint64_t serial{},logical{},submitted{},ticket{},client_at{};
    std::shared_ptr<const anim_studio::Clip> clip;
    float time{};
    bool ready{};
    std::unique_ptr<CustomFlatland> trick;
    bool client_current() const noexcept {
        return client_at && context.now>=client_at && context.now-client_at<150;
    }
    bool seed_current() const noexcept {
        return seed_owner==context.frame.owner && seed_generation==context.frame.owner_generation &&
            seed_episode==context.manual_episode && seed_stance==context.frame.stance_and_push;
    }
    CustomTrickPlayback playback() noexcept {
        return {this,
            [](void* p,std::shared_ptr<const anim_studio::Clip> next_clip)->std::uint64_t {
                auto& s=*static_cast<State*>(p);
                if(!s.ready || !s.client_current() || !s.seed_current() || s.serial==UINT64_MAX)return 0;
                s.clip=std::move(next_clip);s.time=0;return s.logical=++s.serial;
            },
            [](void* p,std::uint64_t id) {
                const auto& s=*static_cast<State*>(p);return id && id==s.logical && s.client_current();
            },
            [](void* p,std::uint64_t id) {
                auto& s=*static_cast<State*>(p);if(id==s.logical){s.logical=0;s.clip.reset();s.time=0;}
            },
            [](void* p,std::uint64_t id,float next_time) {
                auto& s=*static_cast<State*>(p);if(id==s.logical)s.time=next_time;
            }};
    }
};
State& state(){static auto* s=new State;return *s;}
void observe_locked(State& s,CustomFlatlandContext context) noexcept {
    s.context=context;
    if(s.trick)s.trick->observe(context);
    if(!context.valid() || !s.seed_current()) {
        if(s.trick)s.trick->cancel();
        s.ready=false;
    }
}
}
CustomFlatlandBinding observe(CustomFlatlandContext context) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);observe_locked(s,context);
    if(!s.trick || !s.ready || !s.client_current() || !context.valid())return {};
    return {true,s.trick->branch(),s.trick->branch_ready()};
}
InputDisposition request(const InputRequest& request) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    if(!s.trick || !s.ready || !s.client_current())return InputDisposition::unsupported;
    const auto adapter=s.trick->adapter();return adapter.request(adapter.state,request);
}
CustomFlatland::Motion consume(const NativeInputFrame& frame,std::uint64_t now,
        const std::array<float,3>& velocity,bool admitted) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    auto context=s.context;context.frame=frame;context.now=now;
    context.manual_admitted=context.manual_admitted && admitted;
    observe_locked(s,context);
    // An accepted gesture is not proof that the client player accepted its clip.
    if(!s.trick || !s.logical || s.submitted!=s.logical || !s.ticket)return {};
    if(!skater_animation::controlled_play_current(s.ticket)){s.trick->cancel();s.ready=false;return {};}
    return s.trick->step(context,velocity); // unknown support material: conservative default
}
void cancel() noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    if(s.trick)s.trick->cancel();
    s.ready=false;s.context.enabled=false;
}
void tick(CustomFlatlandContext context) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    try {
        observe_locked(s,context); // Expire a missed heartbeat before renewing it.
        if(s.ticket && (s.submitted!=s.logical || !skater_animation::owns_play(s.ticket))) {
            const bool lost=s.submitted==s.logical;
            skater_animation::cancel_play(s.ticket);s.ticket=s.submitted=0;
            if(lost && s.trick)s.trick->cancel();
        }
        s.client_at=context.now;
        if(!context.valid()){s.ready=false;return;}
        if(s.logical && !s.ticket) {
            skater_animation::Play play{};play.clip=s.clip;play.blend=.15f;play.floor=false;
            s.ticket=skater_animation::try_play_controlled(std::move(play));
            if(!s.ticket){s.trick->cancel();s.ready=false;return;}
            s.submitted=s.logical;
        }
        if(s.ticket) {
            if(!skater_animation::seek_owned(s.ticket,s.time)) {
                s.trick->cancel();skater_animation::cancel_play(s.ticket);s.ticket=s.submitted=0;s.ready=false;
            }
            return;
        }
        s.ready=false;
        skater_animation::prepare_playback();
        if(!skater_animation::can_play())return;
        if(s.seed_current() && s.trick){s.ready=true;return;}
        // Require a pose evaluated after this independently admitted episode
        // was first seen. A cached pre-Manual pose cannot seed a new trick.
        if(s.seed_episode!=context.manual_episode || s.seed_owner!=context.frame.owner ||
            s.seed_generation!=context.frame.owner_generation || s.seed_stance!=context.frame.stance_and_push) {
            s.seed_at=context.now;s.seed_step=context.frame.physics_step;
            s.seed_owner=context.frame.owner;s.seed_generation=context.frame.owner_generation;
            s.seed_episode=context.manual_episode;s.seed_stance=context.frame.stance_and_push;s.trick.reset();
        }
        const auto seed=skater_animation::native_pose();
        if(!seed || seed->base!=context.frame.owner.base || seed->entity!=context.frame.owner.entity ||
            context.frame.physics_step<=s.seed_step || context.now<s.seed_at ||
            seed->age*1000>=static_cast<double>(context.now-s.seed_at))return;
        auto clips=make_casper_clips(seed->joints,seed->board);
        s.trick=std::make_unique<CustomFlatland>(std::move(clips),s.playback());
        s.trick->observe(context);s.ready=true;
    } catch(...) {
        if(s.trick)s.trick->cancel();
        if(s.ticket)skater_animation::cancel_play(s.ticket);
        s.ticket=s.submitted=0;s.ready=false;
    }
}
}
#else
namespace dingosdk::arcade_mode::custom_flatland_runtime {
void tick(CustomFlatlandContext) noexcept {}
CustomFlatlandBinding observe(CustomFlatlandContext) noexcept {return {};}
InputDisposition request(const InputRequest&) noexcept {return InputDisposition::unsupported;}
CustomFlatland::Motion consume(const NativeInputFrame&,std::uint64_t,const std::array<float,3>&,bool) noexcept {return {};}
void cancel() noexcept {}
}
#endif
