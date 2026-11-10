#include "arcade_custom_flatland_runtime.h"
#if defined(DINGOSDK_DEV_TOOLS)
#include "arcade_custom_flatland_clips.h"
#include "arcade_native_wall_contact.h"
#include "Extension/Skater/skater_animation.h"
#include <mutex>

namespace dingosdk::arcade_mode::custom_flatland_runtime {
namespace {
struct State {
    std::mutex mutex;
    CustomFlatlandContext context;
    Observation observation{};
    NativeInputFrame spin_input{};
    std::uint64_t spin_at{};
    int spin_direction{};
    NativeInputOwner seed_owner{};
    std::uint64_t seed_generation{},seed_episode{},seed_step{},seed_at{};
    std::uint32_t seed_stance{};
    std::uint64_t serial{},logical{},submitted{},ticket{},client_at{};
    std::shared_ptr<const anim_studio::Clip> clip;
    float time{};
    bool enabled{},ready{};
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
void observe_locked(State& s,CustomFlatlandContext& context) noexcept {
    context.enabled=context.enabled && s.enabled;
    s.context=context;
    if(s.trick)s.trick->observe(context);
    if(!context.valid() || !s.seed_current() ||
        (s.ticket && !skater_animation::controlled_play_current(s.ticket))) {
        if(s.trick)s.trick->cancel();
        s.ready=false;
    }
}
}
void set_enabled(bool enabled) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    if(s.enabled==enabled)return;
    if(s.trick)s.trick->cancel();
    s.enabled=enabled;s.ready=false;s.context.enabled=false;
    native_wall_contact::enable_materials(false);
    if(enabled)s.observation={};
}
Status status() noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    return {s.enabled,s.ready && s.client_current(),
        s.ticket && skater_animation::controlled_play_current(s.ticket),
        s.trick?s.trick->phase():CustomFlatland::Phase::manual,s.observation};
}
CustomFlatlandBinding observe(CustomFlatlandContext context) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);observe_locked(s,context);
    if(!s.trick || !s.ready || !s.client_current() || !context.valid())return {};
    return {true,s.trick->branch(),s.trick->branch_ready()};
}
CustomFlatlandBinding observe_input(CustomFlatlandContext context,unsigned lt,unsigned rt) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    const bool active=s.trick && s.trick->active();
    // A preheld trigger cannot enter Casper. Once acquired, triggers belong
    // to this presentation lease, while native Manual keeps its RS pressure.
    if(!active && (lt>30 || rt>30))context.enabled=false;
    observe_locked(s,context);
    s.spin_input={};s.spin_at=0;s.spin_direction=0;
    if(!s.trick || !s.ready || !s.client_current() || !context.valid())return {};
    const bool owns=s.trick->active();
    if(owns) {
        s.spin_input=context.frame;s.spin_at=context.now;
        s.spin_direction=(lt>30?1:0)-(rt>30?1:0);
    }
    return {true,s.trick->branch(),s.trick->branch_ready(),owns};
}
InputDisposition request(const InputRequest& request) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    if(!s.trick || !s.ready || !s.client_current())return InputDisposition::unsupported;
    const auto adapter=s.trick->adapter();return adapter.request(adapter.state,request);
}
CustomFlatland::Motion consume(const NativeInputFrame& frame,std::uint64_t now,
        const std::array<float,3>& velocity,bool admitted,const FlatlandSurfaceEvidence& surface) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    auto context=s.context;context.frame=frame;context.now=now;
    context.manual_admitted=context.manual_admitted && admitted;
    observe_locked(s,context);
    // An accepted gesture is not proof that the client player accepted its clip.
    if(!s.trick || !s.logical || s.submitted!=s.logical || !s.ticket)return {};
    if(!skater_animation::controlled_play_current(s.ticket)){s.trick->cancel();s.ready=false;return {};}
    context.spin_direction=s.spin_at && now>=s.spin_at && now-s.spin_at<=150 &&
        s.spin_input.owner==frame.owner && s.spin_input.owner_generation==frame.owner_generation &&
        s.spin_input.contact_generation==frame.contact_generation &&
        s.spin_input.action_generation==frame.action_generation &&
        s.spin_input.physics_step<=frame.physics_step ? s.spin_direction : 0;
    const auto motion=s.trick->step(context,velocity,surface);
    if(motion.admitted) {
        auto& row=s.observation;
        row.step=frame.physics_step;row.seconds=frame.physics_seconds;
        row.verified=surface.matches(frame);
        row.surface=row.verified?surface.surface:FlatlandSurface::unknown;
        const auto index=static_cast<std::size_t>(row.surface);
        if(index<row.surface_steps.size() && row.surface_steps[index]!=UINT64_MAX)++row.surface_steps[index];
        row.deceleration=flatland_friction(frame,surface).rolling*60*GameplayProfile::metres_per_inch;
        row.speed=std::hypot(velocity[0],velocity[2]);row.requested_loss=std::hypot(motion.x,motion.z);
        row.display_radians=s.trick->display_radians();
        if(s.trick->phase()==CustomFlatland::Phase::hold) {
            if(context.spin_direction==1)row.left_seconds+=frame.physics_seconds;
            if(context.spin_direction==-1)row.right_seconds+=frame.physics_seconds;
        }
    }
    return motion;
}
void cancel() noexcept {
    native_wall_contact::enable_materials(false);
    auto& s=state();std::lock_guard lock(s.mutex);
    if(s.trick)s.trick->cancel();
    s.ready=false;s.context.enabled=false;
}
void tick(CustomFlatlandContext context) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    try {
        observe_locked(s,context); // Expire a missed heartbeat before renewing it.
        native_wall_contact::enable_materials(s.enabled && context.valid() &&
            native_wall_contact::start_materials(context.frame.owner.base));
        if(s.ticket && !skater_animation::owns_play(s.ticket)) {
            // Losing the old clip also invalidates a queued phase transition.
            // It must never acquire a fresh ticket after another owner wins.
            skater_animation::cancel_play(s.ticket);s.ticket=s.submitted=0;
            if(s.trick)s.trick->cancel();
            s.ready=false;
        }
        if(s.ticket && s.submitted!=s.logical) {
            if(s.logical && skater_animation::replace_owned(s.ticket,s.clip,s.time)) {
                s.submitted=s.logical;
            } else {
                if(s.trick)s.trick->cancel();
                skater_animation::cancel_play(s.ticket);s.ticket=s.submitted=0;
                s.ready=false;
            }
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
            if(!skater_animation::seek_owned(s.ticket,s.time,s.trick->display_radians())) {
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
        native_wall_contact::enable_materials(false);
        if(s.trick)s.trick->cancel();
        if(s.ticket)skater_animation::cancel_play(s.ticket);
        s.ticket=s.submitted=0;s.ready=false;
    }
}
}
#else
namespace dingosdk::arcade_mode::custom_flatland_runtime {
void set_enabled(bool) noexcept {}
Status status() noexcept {return {};}
void tick(CustomFlatlandContext) noexcept {}
CustomFlatlandBinding observe(CustomFlatlandContext) noexcept {return {};}
CustomFlatlandBinding observe_input(CustomFlatlandContext,unsigned,unsigned) noexcept {return {};}
InputDisposition request(const InputRequest&) noexcept {return InputDisposition::unsupported;}
CustomFlatland::Motion consume(const NativeInputFrame&,std::uint64_t,const std::array<float,3>&,bool,const FlatlandSurfaceEvidence&) noexcept {return {};}
void cancel() noexcept {}
}
#endif
