#pragma once
#include "arcade_native_input_state.h"
#include "arcade_surface_geometry.h"
#include <algorithm>
#include <span>

namespace dingosdk::arcade_mode {
namespace vert_math {
using Vec=SurfaceGeometryVector;
using namespace surface_geometry_detail;
inline Vec add(Vec a,Vec b) noexcept {return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
inline Vec scale(Vec v,float s) noexcept {return {v[0]*s,v[1]*s,v[2]*s};}
inline float length(Vec v) noexcept {return std::sqrt(dot(v,v));}
inline Vec horizontal(Vec v) noexcept {v[1]=0;return unit(v)?v:Vec{};}
inline Vec position(Vec p,Vec v,float gravity,float t) noexcept {
    auto result=add(p,scale(v,t));result[1]-=.5f*gravity*t*t;return result;
}
}
enum class VertAction : std::uint8_t { none, level_out, spine, hip, acid_drop };
enum class VertPhase : std::uint8_t { idle, rising_support, native_air, requested, released, landed, cancelled };
// Nine independent static-owner samples cover the board footprint. Shapes and
// materials may differ, but missing support or a discontinuous normal rejects.
// This is geometry evidence only; native contacts still establish support.
struct VertPatch {
    std::array<SurfaceGeometrySample,9> samples{};
    bool valid{};
    const SurfaceGeometrySample& center() const noexcept {return samples[0];}
};
struct VertObservation {
    NativeInputFrame native{};
    SurfaceGeometryVector position{},velocity{},up{0,1,0};
    VertPatch support{};
    bool enabled{}, supported{}, contested{};
    SurfaceGeometryBody contacted_shape{};
};
struct VertPlan {
    VertAction action{};
    SurfaceGeometryVector velocity{},landing{},normal{},direction{};
    VertPatch destination{};
    std::array<SurfaceGeometrySample,3> crest{};
    float seconds{},cost{};
    bool clear{};
    explicit operator bool() const noexcept {return action!=VertAction::none && clear;}
};
// Analytic reachability only, not a replacement integrator. Gravity is measured
// from consecutive original physics observations; native collision keeps control.
inline VertPlan vert_destination(const VertObservation& from,const VertPatch& origin,
        const VertPatch& destination,SurfaceGeometryVector direction,float gravity,float time) noexcept {
    using namespace vert_math;
    VertPlan result;
    if(!origin.valid || !destination.valid || !finite(from.position) || !finite(from.velocity) ||
       !std::isfinite(gravity) || gravity<3 || gravity>60 || !std::isfinite(time) || time<.25f || time>2.5f)return result;
    const auto& target=destination.center();const auto n=target.query_normal;
    if(!finite(target.point) || !finite(n) || std::abs(dot(n,n)-1)>.02f || n[1]<.15f || n[1]>.92f)return result;
    direction=horizontal(direction);if(length(direction)<.9f)return result;
    const auto displacement=sub(target.point,from.position);
    const auto across=horizontal(displacement);
    if(dot(across,direction)<.35f || std::hypot(displacement[0],displacement[2])<.75f)return result;
    auto velocity=scale(displacement,1/time);velocity[1]+=.5f*gravity*time;
    auto arrival=velocity;arrival[1]-=gravity*time;
    const float speed=length(from.velocity),change=length(sub(velocity,from.velocity));
    const float impact=-dot(arrival,n),along=length(sub(arrival,scale(n,dot(arrival,n))));
    // No launch energy injection and no catch by approaching the underside.
    if(speed<2 || length(velocity)>speed*1.05f || change>std::min(8.f,speed*.65f) ||
       arrival[1]>=-.5f || impact<.1f || impact>std::min(6.f,along*.65f) || along<2)return result;
    auto downhill=horizontal(n);
    if(dot(horizontal(arrival),downhill)<.5f)return result;
    const float alignment=dot(horizontal(origin.center().query_normal),downhill);
    const float drop=origin.center().point[1]-target.point[1];
    if(from.velocity[1]<-.5f && drop>.75f)result.action=VertAction::acid_drop;
    else if(alignment<-.7f)result.action=VertAction::spine;
    else if(alignment<.7f)result.action=VertAction::hip;
    else if(drop>.75f)result.action=VertAction::acid_drop;
    else return {};
    result.velocity=velocity;result.landing=target.point;result.normal=n;
    result.direction=horizontal(arrival);result.destination=destination;result.seconds=time;
    // Intent and change in momentum outweigh distance. The caller samples a
    // fixed travel-relative fan, so equivalent rotated geometry has the same order.
    result.cost=change+4*(1-dot(across,direction))+.1f*time;
    return result;
}
inline VertPlan vert_level_out(const VertObservation& from,SurfaceGeometryVector direction) noexcept {
    using namespace vert_math;
    VertPlan result;direction=horizontal(direction);
    const float speed=length(from.velocity);
    if(!finite(from.velocity) || speed<2 || speed>50 || length(direction)<.9f)return result;
    // Redistribute a bounded part of upward speed into forward motion. Downward
    // momentum and gravity remain native; this never adds a second jump.
    const float vy=std::min(from.velocity[1],speed*.35f);
    const float horizontal_speed=std::sqrt(std::max(0.f,speed*speed-vy*vy));
    result.velocity={direction[0]*horizontal_speed,vy,direction[2]*horizontal_speed};
    const auto delta=sub(result.velocity,from.velocity);const float change=length(delta);
    const float budget=std::min(8.f,speed*.65f);
    if(change>budget)result.velocity=add(from.velocity,scale(delta,budget/change));
    result.action=VertAction::level_out;result.direction=direction;result.normal={0,1,0};
    return result;
}
inline bool vert_align_target(std::array<float,16>& target,SurfaceGeometryVector normal,float seconds) noexcept {
    using namespace vert_math;
    if(!finite(normal) || !unit(normal) || !std::isfinite(seconds) || seconds<=0 || seconds>.1f)return false;
    Vec up{target[4],target[5],target[6]};if(!unit(up))return false;
    auto axis=cross(up,normal);const float cosine=std::clamp(dot(up,normal),-1.f,1.f);
    if(cosine>.99999f || !unit(axis))return false;
    const float angle=std::min(std::acos(cosine),2.5f*seconds),c=std::cos(angle),s=std::sin(angle);
    for(unsigned row:{0u,4u,8u}) {
        const Vec v{target[row],target[row+1],target[row+2]};
        const auto turned=add(add(scale(v,c),scale(cross(axis,v),s)),scale(axis,dot(axis,v)*(1-c)));
        for(unsigned i=0;i<3;++i)target[row+i]=turned[i];
    }
    return true;
}
class VertAirController {
public:
    void cancel() noexcept {phase_=VertPhase::cancelled;request_=0;transition_at_=0;plan_={};released_plan_={};}
    void reset() noexcept {*this={};}
    // A busy input publisher is absence of evidence, not a new contact. Keep
    // recent curvature for this physical owner, but withdraw every command and
    // require a newer admitted frame before input, queries or writes can resume.
    void unavailable(std::uint64_t now,const NativeInputOwner& owner,bool enabled,bool contested) noexcept {
        plan_={};
        unavailable_at_=std::max(unavailable_at_,now);
        if(!enabled || contested || !owner.valid() || !previous_.native.valid ||
           owner!=previous_.native.owner || now<previous_.native.sampled_at ||
           now-previous_.native.sampled_at>100 || phase_==VertPhase::released) {
            cancel();previous_={};
        }
    }
    bool matches(const NativeInputFrame& f,std::uint64_t now) const noexcept {
        return f.valid && f.sampled_at>unavailable_at_ && f.owner==takeoff_.native.owner && f.owner_generation==takeoff_.native.owner_generation &&
            f.airborne && f.airborne_episode==takeoff_.native.airborne_episode &&
            f.air_departure==takeoff_.native.air_departure && now>=f.sampled_at && now-f.sampled_at<=150;
    }
    bool eligible(const NativeInputFrame& f,std::uint64_t now) const noexcept {
        return phase_==VertPhase::native_air && matches(f,now) && now>=takeoff_.native.sampled_at &&
            now-takeoff_.native.sampled_at<=4000;
    }
    void observe(const VertObservation& value) noexcept {
        using namespace vert_math;
        const auto& f=value.native;
        if(!value.enabled || !f.valid || !f.owner.valid() || value.contested || !finite(value.position) ||
           !finite(value.velocity) || !std::isfinite(f.physics_seconds) || f.physics_seconds<.001f || f.physics_seconds>.1f) {
            cancel();previous_={};return;
        }
        if(f.sampled_at<=unavailable_at_)return;
        if(previous_.native.valid && (f.owner!=previous_.native.owner || f.owner_generation!=previous_.native.owner_generation ||
           f.sampled_at<previous_.native.sampled_at || f.sampled_at-previous_.native.sampled_at>150)) {
            reset();
        }
        if(f.physics_step<=previous_.native.physics_step)return;
        if(!f.airborne) {
            // Only a real supported riding observation records landing. A query
            // destination or a state departure on its own cannot do so.
            const bool landed=matches(previous_.native,previous_.native.sampled_at) && value.supported &&
                (f.state==100 || f.state==103) && phase_==VertPhase::released &&
                (released_plan_.action==VertAction::level_out ||
                 (length(sub(value.position,released_plan_.landing))<=1.5f &&
                  dot(value.up,released_plan_.normal)>.7f &&
                  std::any_of(released_plan_.destination.samples.begin(),released_plan_.destination.samples.end(),
                    [&](const auto& sample){return sample.body==value.contacted_shape;})));
            request_=0;plan_={};gravity_=0;gravity_samples_=0;
            const auto n=value.support.center().query_normal;
            const auto& before=previous_.support;
            // A steep plane alone is not a transition. Observe the concave
            // change of support normal while physically climbing the ramp.
            if(value.support.valid && value.supported && before.valid && previous_.supported &&
               !previous_.native.airborne && f.sampled_at>=previous_.native.sampled_at &&
               f.sampled_at-previous_.native.sampled_at<=100 && n[1]<before.center().query_normal[1]-.005f &&
               dot(sub(n,before.center().query_normal),value.velocity)<-.05f &&
               value.position[1]>previous_.position[1]+.005f &&
               length(sub(value.position,previous_.position))<3)transition_at_=f.sampled_at;
            const bool curved=transition_at_ && f.sampled_at>=transition_at_ && f.sampled_at-transition_at_<=500;
            const bool steep=curved &&value.support.valid && value.supported && (f.state==100 || f.state==103) &&
                f.mode<=2 && n[1]>=-.01f && n[1]<.55f && value.velocity[1]>2 &&
                dot(value.up,n)>.7f && std::abs(dot(value.velocity,n))<2;
            if(steep) {origin_=value.support;lip_=value;phase_=VertPhase::rising_support;}
            else {
                // Wheel contact can end a few original steps before AIR is
                // published. Preserve that departure, not a new launch: its
                // deadline remains anchored to the last real curved support.
                const bool departing=phase_==VertPhase::rising_support && (!value.supported || !value.support.valid) &&
                    lip_.native.valid && f.owner==lip_.native.owner && f.owner_generation==lip_.native.owner_generation &&
                    f.sampled_at>=lip_.native.sampled_at && f.sampled_at-lip_.native.sampled_at<=100 &&
                    (f.state==100 || f.state==103) && f.mode<=2 && !f.rail_family && value.velocity[1]>2 &&
                    dot(value.up,lip_.support.center().query_normal)>.7f && length(sub(value.position,lip_.position))<2;
                phase_=departing?VertPhase::rising_support:landed?VertPhase::landed:VertPhase::idle;
            }
        } else if(!previous_.native.airborne) {
            if(phase_==VertPhase::rising_support && previous_.native.valid &&
               f.airborne_started_known && lip_.native.valid && f.airborne_started_at>=lip_.native.sampled_at &&
               f.sampled_at>=lip_.native.sampled_at && f.sampled_at-lip_.native.sampled_at<=100 && value.velocity[1]>2 && !f.rail_family && f.mode<=2) {
                takeoff_=value;phase_=VertPhase::native_air;
                direction_=scale(horizontal(origin_.center().query_normal),-1);
            } else cancel();
        } else if((phase_==VertPhase::native_air || phase_==VertPhase::requested || phase_==VertPhase::released) &&
                  !matches(f,f.sampled_at))cancel();
        if(phase_==VertPhase::released && previous_.native.airborne && f.physics_step==previous_.native.physics_step+1) {
            const auto delta=sub(value.velocity,previous_.velocity);
            if(std::hypot(delta[0],delta[2])>.3f || std::abs(delta[1]+gravity_*f.physics_seconds)>.5f)cancel();
        }
        if((phase_==VertPhase::native_air || phase_==VertPhase::requested) && previous_.native.airborne &&
           f.physics_step==previous_.native.physics_step+1) {
            const auto dv=sub(previous_.velocity,value.velocity);
            const float measured=dv[1]/f.physics_seconds;
            if(std::hypot(dv[0],dv[2])<.15f && measured>=3 && measured<=60) {
                if(!gravity_samples_ || std::abs(measured-gravity_)<2) {
                    gravity_=(gravity_*static_cast<float>(gravity_samples_)+measured)/static_cast<float>(gravity_samples_+1);
                    gravity_samples_=std::min(gravity_samples_+1,16u);
                } else {gravity_=measured;gravity_samples_=1;}
            } else {gravity_samples_=0;gravity_=0;}
        }
        previous_=value;
    }
    bool request(const NativeInputFrame& f,std::uint64_t now,std::uint64_t edge,std::uint64_t sequence,
                 SurfaceGeometryVector intent={}) noexcept {
        if(!sequence || !eligible(f,now) || edge<takeoff_.native.sampled_at || now<edge || now-edge>150)return false;
        request_=sequence;request_at_=edge;phase_=VertPhase::requested;
        const auto selected=vert_math::horizontal(intent);
        if(vert_math::length(selected)>.9f && vert_math::dot(selected,direction_)>.0f)direction_=selected;
        return true;
    }
    bool pending(const NativeInputFrame& f,std::uint64_t now) const noexcept {
        return phase_==VertPhase::requested && request_ && now>=request_at_ && now-request_at_<=150 && matches(f,now);
    }
    bool stage(VertPlan plan,const VertObservation& source,std::uint64_t now) noexcept {
        if(!pending(source.native,now) || !plan || !finite_motion(source) ||
           !source.native.flip_known || source.native.flip_id!=-1)return false;
        plan_=plan;planned_=source.native;planned_velocity_=source.velocity;planned_position_=source.position;return true;
    }
    VertPlan consume(const NativeInputFrame& f,std::uint64_t now,SurfaceGeometryVector velocity) noexcept {
        if(!pending(f,now) || !plan_ || planned_.owner!=f.owner || f.physics_step<planned_.physics_step ||
           now<planned_.sampled_at || now-planned_.sampled_at>50) {
            if(request_ && now>=request_at_ && now-request_at_>150)cancel();return {};
        }
        // A newly selected native trick owns its physical trajectory even if
        // its first velocity still matches the query source. Keep the original
        // short input lifetime, so retirement cannot replay an expired command.
        if(!f.flip_known || f.flip_id!=-1) {plan_={};return {};}
        // Queries may finish between adjacent original physics steps. Compare
        // against that immutable source, never against a newer observation that
        // would hide a collision. Carry only its velocity delta across gravity.
        const float elapsed=static_cast<float>(f.physics_step-planned_.physics_step)*planned_.physics_seconds;
        auto expected=planned_velocity_;expected[1]-=gravity_*elapsed;
        if(elapsed>.05f || std::abs(f.physics_seconds-planned_.physics_seconds)>.00001f ||
           vert_math::length(vert_math::sub(velocity,expected))>.3f ||
           vert_math::length(vert_math::sub(previous_.position,
               vert_math::position(planned_position_,planned_velocity_,gravity_,elapsed)))>.25f) {
            cancel();return {};
        }
        auto result=plan_;
        result.velocity=vert_math::add(velocity,vert_math::sub(result.velocity,planned_velocity_));
        released_plan_=result;released_at_=now;
        plan_={};request_=0;phase_=VertPhase::released;previous_.velocity=result.velocity;return result;
    }
    SurfaceGeometryVector alignment(const NativeInputFrame& f,std::uint64_t now) const noexcept {
        if(phase_!=VertPhase::released || !matches(f,now) || now<released_at_ || now-released_at_>2500 ||
           !f.flip_known || f.flip_id!=-1 || f.mode>2)return {};
        return released_plan_.normal;
    }
    const VertObservation& observation() const noexcept {return previous_;}
    const VertPatch& origin() const noexcept {return origin_;}
    const VertPlan& released_plan() const noexcept {return released_plan_;}
    SurfaceGeometryVector direction() const noexcept {return direction_;}
    float gravity() const noexcept {return gravity_samples_>=2?gravity_:0;}
    VertPhase phase() const noexcept {return phase_;}
private:
    VertPhase phase_{};
    VertObservation previous_{},takeoff_{},lip_{};
    VertPatch origin_{};
    SurfaceGeometryVector direction_{};
    VertPlan plan_{},released_plan_{};
    std::uint64_t released_at_{},unavailable_at_{};
    NativeInputFrame planned_{};
    static bool finite_motion(const VertObservation& o) noexcept {
        return vert_math::finite(o.position) && vert_math::finite(o.velocity);
    }
    SurfaceGeometryVector planned_velocity_{},planned_position_{};
    std::uint64_t request_{},request_at_{},transition_at_{};
    float gravity_{};
    unsigned gravity_samples_{};
};
}
