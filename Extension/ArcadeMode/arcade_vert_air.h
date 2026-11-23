#pragma once
#include "arcade_native_input_state.h"
#include "arcade_input_policy.h"
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
enum class VertPhase : std::uint8_t { idle, rising_support, native_air, requested, released, landed, cancelled, native_owned };
// Nine independent static-owner samples cover the board footprint. Shapes and
// materials may differ, but missing support or a discontinuous normal rejects.
// This is geometry evidence only; native contacts still establish support.
struct VertPatch {
    std::array<SurfaceGeometrySample,9> samples{};
    bool valid{};
    const SurfaceGeometrySample& center() const noexcept {return samples[0];}
};
struct VertRampContact {
    SurfaceGeometryVector point{},normal{};
    SurfaceGeometryBody shape{};
    SurfaceGeometryWorldOwner owner{};
};
// Origin geometry can come from independently validated original wheel
// contacts. Destination admission still requires its complete query footprint.
struct VertRampSupport {
    SurfaceGeometryVector point{},normal{};
    std::array<VertRampContact,2> contacts{};
    bool valid{};
    VertRampSupport()=default;
    VertRampSupport(const VertPatch& patch) noexcept : point(patch.center().point),
        normal(patch.center().query_normal),valid(patch.valid) {
        for(std::size_t i=0;i<contacts.size();++i) {
            const auto& sample=patch.samples[i];contacts[i]={sample.point,sample.query_normal,sample.body,sample.world_owner};
        }
    }
};
inline VertRampSupport vert_ramp_contacts(std::span<const VertRampContact> contacts,
        SurfaceGeometryVector board,SurfaceGeometryVector up) noexcept {
    using namespace vert_math;
    if(!finite(board) || !finite(up))return {};
    for(std::size_t i=0;i<contacts.size();++i)for(std::size_t j=i+1;j<contacts.size();++j) {
        const auto& first=contacts[i];const auto& second=contacts[j];
        const auto qualifies=[&](const auto& c) {
            return c.shape.world && c.shape.index!=UINT32_MAX && c.owner.value && finite(c.point) && finite(c.normal) &&
                std::abs(dot(c.normal,c.normal)-1)<.02f && c.normal[1]>=-.01f && dot(c.normal,up)>.7f &&
                length(sub(c.point,board))<1.2f;
        };
        const float separation=length(sub(first.point,second.point));
        if(!qualifies(first) || !qualifies(second) || first.shape.world!=second.shape.world ||
           separation<.15f || separation>1.5f || dot(first.normal,second.normal)<.9f)continue;
        VertRampSupport result;result.point=scale(add(first.point,second.point),.5f);
        result.normal=add(first.normal,second.normal);if(!unit(result.normal))continue;
        result.contacts={first,second};result.valid=true;return result;
    }
    return {};
}
// A lip can retain only the rear or front wheel pair before native AIR. The
// bits permit inspecting original contacts; geometry and owners still prove
// support, and an admitted Manual remains a competing owner.
inline bool vert_riding_contact_mask(std::uint32_t state,std::uint32_t contacts) noexcept {
    return (state==100 || state==103) && (contacts==0x4000u || contacts==0x8000u || contacts==0xc000u);
}
struct VertObservation {
    NativeInputFrame native{};
    SurfaceGeometryVector position{},velocity{},up{0,1,0};
    VertPatch support{};
    VertRampSupport native_ramp{};
    VertRampSupport ramp() const noexcept {return native_ramp.valid?native_ramp:VertRampSupport{support};}
    bool enabled{}, supported{}, contested{};
    SurfaceGeometryBody contacted_shape{};
    SurfaceGeometryVector contact_normal{},contact_point{};
    SurfaceGeometryWorldOwner contact_owner{};
};
// Recognition evidence only. The trigger journal owns the physical edge and
// the existing native Revert actuator owns movement. A predicted approach may
// reserve input, but only original wheel contact opens execution permission.
class VertLandingWindow {
public:
    static constexpr std::uint64_t before_ms=180,after_ms=250;
    void reset() noexcept {*this={};}
    void begin(const VertObservation& value) noexcept {reset();takeoff_=last_air_=value;}
    bool flight(const NativeInputFrame& f) const noexcept {
        return takeoff_.native.valid && f.valid && f.owner==takeoff_.native.owner &&
            f.owner_generation==takeoff_.native.owner_generation && f.airborne &&
            f.airborne_episode==takeoff_.native.airborne_episode && f.air_departure==takeoff_.native.air_departure;
    }
    void observe(const VertObservation& value) noexcept {
        using namespace vert_math;
        if(!takeoff_.native.valid)return;
        const auto& f=value.native;
        const auto& previous=last_air_.native;
        if(!value.enabled || value.contested || !f.valid || !finite(value.position) || !finite(value.velocity) ||
           f.owner!=takeoff_.native.owner || f.owner_generation!=takeoff_.native.owner_generation ||
           f.rail_family || f.mode>2 || f.sampled_at<previous.sampled_at ||
           f.sampled_at-takeoff_.native.sampled_at>5000) {reset();return;}
        if(f.physics_step<=previous.physics_step)return;
        if(f.airborne) {
            if(contact_.valid || !flight(f) || f.sampled_at-previous.sampled_at>100)reset();
            else last_air_=value;
            return;
        }
        if(f.airborne_episode!=previous.airborne_episode || f.air_departure!=previous.air_departure+1 ||
           (f.state!=100 && f.state!=103 && f.state!=101)) {reset();return;}
        if(contact_.valid) {
            if(f.sampled_at-contact_.sampled_at>after_ms)reset();
            return;
        }
        if(f.sampled_at-previous.sampled_at>100 || f.sampled_at-takeoff_.native.sampled_at<150 ||
           last_air_.velocity[1]>=-.1f) {reset();return;}
        // State publication may lead contact by an original step. It cannot
        // extend the last AIR timestamp or establish landing by itself.
        if(!value.supported)return;
        auto normal=value.contact_normal;
        // A curved transition can carry speed mostly vertically at touchdown.
        // Require motion along its actual support plane, not world-horizontal
        // speed or impact velocity into that plane (5f9 original callback6672).
        if((f.state!=100 && f.state!=103) || !unit(normal) || normal[1]<.1f ||
           dot(value.up,normal)<.5f || length(sub(value.velocity,scale(normal,dot(value.velocity,normal))))<1 ||
           !value.contacted_shape.world) {reset();return;}
        contact_=f;
    }
    bool approach(const NativeInputFrame& source,const VertPatch& patch) noexcept {
        using namespace vert_math;
        if(!flight(source) || source.physics_step!=last_air_.native.physics_step || !patch.valid ||
           last_air_.velocity[1]>=-.1f || source.sampled_at-takeoff_.native.sampled_at<150)return false;
        const auto normal=patch.center().query_normal;
        if(!finite(normal) || normal[1]<.1f || std::abs(dot(normal,normal)-1)>.02f)return false;
        const float distance=dot(sub(last_air_.position,patch.center().point),normal);
        const float closing=-dot(last_air_.velocity,normal);
        if(!std::isfinite(distance) || distance<0 || distance>1.5f || closing<.2f ||
           distance/closing>static_cast<float>(before_ms)*.001f)return false;
        approach_=source;return true;
    }
    bool available(const NativeInputFrame& current,std::uint64_t now) const noexcept {
        if(!takeoff_.native.valid || !current.valid || current.owner!=takeoff_.native.owner ||
           current.owner_generation!=takeoff_.native.owner_generation || now<current.sampled_at ||
           now-current.sampled_at>100 || current.rail_family || current.mode>2)return false;
        if(contact_.valid)return !current.airborne && current.physics_step>=contact_.physics_step &&
            current.airborne_episode==contact_.airborne_episode && current.air_departure==contact_.air_departure &&
            (current.state==100 || current.state==103 || current.state==101) &&
            now>=contact_.sampled_at && now-contact_.sampled_at<=after_ms;
        return flight(current) && approach_.valid && now>=approach_.sampled_at &&
            now-approach_.sampled_at<=100 && current.physics_step>=approach_.physics_step;
    }
    std::uint64_t deadline(const InputNativeContext& origin,std::uint64_t edge,
            const NativeInputFrame& current,std::uint64_t now) const noexcept {
        if(!available(current,now) || !origin.required || !origin.fresh(edge) ||
           origin.generations.owner!=takeoff_.native.owner_generation ||
           origin.physics_step<takeoff_.native.physics_step || edge<takeoff_.native.sampled_at ||
           now<edge || now-edge>before_ms+after_ms || origin.rail_family ||
           origin.airborne_episode!=takeoff_.native.airborne_episode)return 0;
        if(origin.airborne) {
            if(origin.air_departure!=takeoff_.native.air_departure)return 0;
        } else if(!contact_.valid || origin.air_departure!=contact_.air_departure ||
                  (origin.flip.state!=100 && origin.flip.state!=103))return 0;
        if(contact_.valid) {
            if((edge<contact_.sampled_at && contact_.sampled_at-edge>before_ms) ||
               (edge>=contact_.sampled_at && edge-contact_.sampled_at>after_ms))return 0;
            return std::min(edge+before_ms+after_ms,contact_.sampled_at+after_ms);
        }
        // Reserve only a fresh AIR edge near the corroborated approach. Its
        // bounded expiry belongs to that original press, never to a later poll.
        if(!origin.airborne || (edge<approach_.sampled_at ? approach_.sampled_at-edge : edge-approach_.sampled_at)>100)return 0;
        return edge+before_ms+after_ms;
    }
    bool contact_publication_pending(const InputNativeContext& current,std::uint64_t now) const noexcept {
        const auto& air=last_air_.native;
        return !contact_.valid && air.valid && current.fresh(now) && !current.airborne && !current.rail_family &&
            current.generations.owner==air.owner_generation && current.physics_step>air.physics_step &&
            current.physics_step-air.physics_step<=2 && current.sampled_at>=air.sampled_at &&
            current.sampled_at-air.sampled_at<=100 && current.airborne_episode==air.airborne_episode &&
            current.air_departure==air.air_departure+1 && (current.flip.state==100 || current.flip.state==103) &&
            current.flip.mode<=2 && last_air_.velocity[1]<-.1f &&
            current.sampled_at-takeoff_.native.sampled_at>=150;
    }
    bool ready(const NativeInputFrame& current,std::uint64_t now) const noexcept {
        return contact_.valid && available(current,now) && current.contacts==0xc000u &&
            current.state==100 && current.mode<=1;
    }
    const NativeInputFrame& takeoff() const noexcept {return takeoff_.native;}
    const NativeInputFrame& contact() const noexcept {return contact_;}
    const NativeInputFrame& approach_frame() const noexcept {return approach_;}
private:
    VertObservation takeoff_{},last_air_{};
    NativeInputFrame approach_{},contact_{};
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
inline VertPlan vert_destination(const VertObservation& from,const VertRampSupport& origin,
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
    // Retain the bounded speed tolerance and reject underside approaches.
    // Initial speed alone cannot reject a reachable drop: gravity supplies
    // the independently checked approach speed at the destination.
    if(length(velocity)>speed*1.05f || change>std::min(8.f,speed*.65f) ||
       arrival[1]>=-.5f || impact<.1f || impact>std::min(6.f,along*.65f) || along<2)return result;
    auto downhill=horizontal(n);
    if(dot(horizontal(arrival),downhill)<.5f)return result;
    const float alignment=dot(horizontal(origin.normal),downhill);
    const float drop=origin.point[1]-target.point[1];
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
    if(!finite(from.velocity) || speed>50 || length(direction)<.9f)return result;
    // Redirect a bounded part of vertical motion into forward motion on either
    // side of the apex. Preserve its sign, kinetic bound and native gravity.
    // Exact rest permits physical leveling without inventing launch momentum.
    const float vy=std::clamp(from.velocity[1],-speed*.35f,speed*.35f);
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
    void cancel() noexcept {publication_frame_={};landing_gap_frame_={};push_gap_selection_=0;mode_gap_selection_=0;suspended_=false;landing_.reset();phase_=VertPhase::cancelled;request_=0;transition_at_=0;plan_={};released_plan_={};}
    void reset() noexcept {*this={};}
    // A busy input publisher is absence of evidence, not a new contact. Keep
    // recent curvature for this physical owner, but withdraw transfer queries
    // and execution until a newer admitted frame proves continuation. Independent
    // landing input can remain reserved; actual contact alone permits Revert.
    void unavailable(std::uint64_t now,const NativeInputOwner& owner,bool enabled,bool contested) noexcept {
        plan_={};
        unavailable_at_=std::max(unavailable_at_,now);
        if(!enabled || contested || !owner.valid() || !previous_.native.valid ||
           (landing_gap_frame_.valid && (now<landing_gap_frame_.sampled_at || now-landing_gap_frame_.sampled_at>100)) ||
           owner!=previous_.native.owner || now<previous_.native.sampled_at ||
           now-previous_.native.sampled_at>100) {
            cancel();previous_={};
        } else if(phase_==VertPhase::released)suspended_=true;
    }
    // The native publisher and body writer can straddle a contact/state edge.
    // This retains history only. It cannot manufacture an admitted input frame,
    // refresh a launch deadline, execute a plan, or establish a landing.
    bool publication_gap(const NativeInputCorrelation& correlation,const NativeRequestSample& live,
            const NativeInputOwner& owner,std::uint64_t now,const char** rejection=nullptr) noexcept {
        const auto reject=[&](const char* reason) {if(rejection)*rejection=reason;return false;};
        if(rejection)*rejection="transition_kind";
        using D=NativeInputCorrelation;
        const auto& f=correlation.observed;
        constexpr auto allowed=D::state_changed|D::contacts_changed|D::mode_changed|D::stance_changed;
        if(!correlation.mismatches || (correlation.mismatches&~allowed))return reject("mismatch_kind");
        if(!f.valid || !live.valid || !previous_.native.valid)return reject("missing_observation");
        if(!owner.valid() || f.owner!=owner || owner!=previous_.native.owner)return reject("owner");
        if(f.owner_generation!=previous_.native.owner_generation)return reject("generation");
        if(f.physics_step<previous_.native.physics_step)return reject("step_reversed");
        if(f.physics_step-previous_.native.physics_step>2) {
            // Several consecutive original/live contact edges can withhold a
            // correlated body observation. Each newer original must confirm the
            // preceding live edge before it can continue this history. The
            // physical sample and last curved support keep their original100ms
            // deadlines; this never admits input, motion or a takeoff itself.
            const auto& prior=publication_frame_;
            if(phase_!=VertPhase::rising_support || !prior.valid)return reject("original_step_gap");
            if(f.owner!=prior.owner || f.owner_generation!=prior.owner_generation)return reject("publication_owner");
            if(f.physics_step<=prior.physics_step || f.physics_step-prior.physics_step>2 ||
               f.sampled_at<=prior.sampled_at)return reject("publication_order");
            if(f.sampled_at<previous_.native.sampled_at || f.sampled_at-previous_.native.sampled_at>100 ||
               static_cast<double>(f.physics_step-previous_.native.physics_step)*f.physics_seconds>.100001)
                return reject("publication_deadline");
            if(!std::isfinite(f.physics_seconds) ||
               std::abs(f.physics_seconds-previous_.native.physics_seconds)>=.00001f)return reject("publication_cadence");
            if(f.airborne || f.rail_family || (f.state!=100 && f.state!=103) ||
               f.airborne_episode!=previous_.native.airborne_episode ||
               f.air_departure!=previous_.native.air_departure ||
               f.airborne_episode!=prior.airborne_episode || f.air_departure!=prior.air_departure)
                return reject("publication_lineage");
            if(f.state!=publication_live_.state || f.mode!=publication_live_.crouch_mode ||
               f.contacts!=(publication_live_.words[1]&0xc000u) ||
               f.stance_and_push!=(publication_live_.words[0]&0x60u))return reject("publication_confirmation");
            if(!f.flip_known || f.flip_id!=-1 || f.selection_object!=prior.selection_object ||
               f.selection_object!=previous_.native.selection_object)return reject("publication_actor");
        }
        if(now<f.sampled_at || now-f.sampled_at>100)return reject("original_age");
        if(f.mode>2)return reject("original_mode");
        const auto stance=live.words[0]&0x60u;
        const bool push_changed=stance!=f.stance_and_push;
        const auto contacts=live.words[1]&0xc000u;
        // Idle AIR can finish a push while entering mode2. Preserve only its
        // existing flight history; both actor and cleared push need a fresh
        // original publication before any trigger or trajectory is eligible.
        const bool idle_mode=correlation.mismatches==(D::mode_changed|(push_changed?D::stance_changed:0u)) &&
            phase_==VertPhase::native_air && same_flight(f) && f.state==201 && live.state==201 &&
            f.mode==1 && live.crouch_mode==2 && !f.contacts && !contacts && !f.rail_family &&
            (!push_changed || ((f.stance_and_push^stance)==0x40u && !(stance&0x40u))) &&
            f.flip_known && f.flip_id==-1 && f.selection_object &&
            previous_.native.flip_known && previous_.native.flip_id==-1 &&
            previous_.native.selection_object==f.selection_object;
        if(idle_mode) {
            mode_gap_selection_=f.selection_object;
            if(push_changed) {push_gap_selection_=f.selection_object;push_gap_stance_=stance;}
            return true;
        }
        if(push_changed) {
            // Native pushing can start or finish on an already proven curved
            // ramp. Neither edge changes leading foot or admits a new launch.
            // The next original frame must confirm this push state and actor.
            if(!(correlation.mismatches&D::stance_changed) ||
               (correlation.mismatches&D::mode_changed) || phase_!=VertPhase::rising_support ||
               (f.state!=100 && f.state!=103) || f.rail_family ||
               (f.stance_and_push^stance)!=0x40u ||
               !f.flip_known || f.flip_id!=-1 || !f.selection_object ||
               !previous_.native.flip_known || previous_.native.flip_id!=-1 ||
               previous_.native.selection_object!=f.selection_object)return false;
        } else if(correlation.mismatches&D::stance_changed)return false;
        if(correlation.mismatches&D::mode_changed) {
            // Riding can publish in the same live observation as wheel contact
            // and mode1. It withdraws flight authority just like partial AIR
            // contact; only a subsequent original supported frame proves landing.
            const bool riding=live.state==100 || live.state==103;
            const auto touchdown_changes=D::mode_changed|D::contacts_changed|(riding?D::state_changed:0u);
            const bool touchdown=correlation.mismatches==touchdown_changes &&
                (phase_==VertPhase::native_air || phase_==VertPhase::requested || phase_==VertPhase::released ||
                 phase_==VertPhase::native_owned) &&
                same_flight(f) && landing_.flight(f) && f.state==201 && (live.state==201 || riding) &&
                f.mode==2 && live.crouch_mode==1 && !f.contacts &&
                (contacts==0x4000u || contacts==0x8000u || contacts==0xc000u) &&
                f.sampled_at>=takeoff_.native.sampled_at && f.sampled_at-takeoff_.native.sampled_at>=150 &&
                previous_.velocity[1]<-.1f && f.stance_and_push==previous_.native.stance_and_push &&
                f.flip_known && f.flip_id==-1 && f.selection_object && previous_.native.flip_known &&
                previous_.native.flip_id==-1 && previous_.native.selection_object==f.selection_object;
            if(touchdown) {
                // Contact publication withdraws transfer motion immediately. It
                // preserves only the prior landing lineage until original support.
                landing_gap_frame_=f;mode_gap_selection_=0;phase_=VertPhase::cancelled;
                plan_={};released_plan_={};request_=0;suspended_=false;
            }
            return touchdown;
        }
        if(live.crouch_mode!=f.mode)return false;
        const auto changed=(live.state!=f.state?D::state_changed:0u)|(contacts!=f.contacts?D::contacts_changed:0u);
        if((changed|(push_changed?D::stance_changed:0u))!=correlation.mismatches)return false;
        const bool ground=f.state==100 || f.state==103;
        const bool air=f.state==200 || f.state==201;
        if(ground) {
            const bool legal=live.state==f.state || (live.state==200 && !contacts);
            if(legal && push_changed) {push_gap_selection_=f.selection_object;push_gap_stance_=stance;}
            if(legal && phase_==VertPhase::rising_support && !f.rail_family && !f.airborne &&
               f.flip_known && f.flip_id==-1 && f.selection_object &&
               previous_.native.flip_known && previous_.native.flip_id==-1 &&
               previous_.native.selection_object==f.selection_object) {
                publication_frame_=f;publication_live_=live;
            } else publication_frame_={};
            return legal;
        }
        return air && ((live.state==f.state && !contacts) ||
            (f.state==200 && live.state==201 && !contacts) || vert_riding_contact_mask(live.state,contacts));
    }
    bool preserves_native() const noexcept {
        return previous_.native.valid && (phase_==VertPhase::rising_support || phase_==VertPhase::native_air ||
            phase_==VertPhase::requested || phase_==VertPhase::released || phase_==VertPhase::native_owned || landing_gap_frame_.valid);
    }
    bool same_flight(const NativeInputFrame& f) const noexcept {
        return f.valid && f.owner==takeoff_.native.owner && f.owner_generation==takeoff_.native.owner_generation &&
            f.airborne && f.airborne_episode==takeoff_.native.airborne_episode && f.air_departure==takeoff_.native.air_departure;
    }
    bool matches(const NativeInputFrame& f,std::uint64_t now) const noexcept {
        return same_flight(f) && f.sampled_at>unavailable_at_ && now>=f.sampled_at && now-f.sampled_at<=150;
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
        publication_frame_={};
        if(previous_.native.valid && unavailable_at_>previous_.native.sampled_at &&
           f.sampled_at-previous_.native.sampled_at>100) {cancel();previous_={};}
        if(landing_gap_frame_.valid) {
            const auto& gap=landing_gap_frame_;
            const bool contact=f.contacts==0x4000u || f.contacts==0x8000u || f.contacts==0xc000u;
            const bool resumed=f.owner==gap.owner && f.owner_generation==gap.owner_generation &&
                f.sampled_at>=gap.sampled_at && f.sampled_at-gap.sampled_at<=100 &&
                f.airborne_episode==gap.airborne_episode && !f.rail_family && f.mode==1 &&
                f.stance_and_push==gap.stance_and_push && f.flip_known && f.flip_id==-1 &&
                f.selection_object==gap.selection_object && contact &&
                (f.airborne?(f.state==201 && f.air_departure==gap.air_departure):
                    ((f.state==100 || f.state==103) && f.air_departure==gap.air_departure+1));
            if(!resumed) {cancel();previous_={};return;}
            // Keep the last pre-contact descent witness. Native contact may
            // already have arrested vertical velocity before riding publishes.
            if(f.airborne) {previous_=value;return;}
            landing_gap_frame_={};
        }
        if(push_gap_selection_) {
            const bool resumed=f.flip_known && f.flip_id==-1 && f.selection_object==push_gap_selection_ &&
                f.stance_and_push==push_gap_stance_ && !f.rail_family && f.mode<=2 &&
                (f.state==100 || f.state==103 || f.state==200 || f.state==201);
            push_gap_selection_=0;
            if(!resumed) {cancel();previous_={};return;}
        }
        if(mode_gap_selection_) {
            const bool resumed=same_flight(f) && f.state==201 && f.mode==2 && !f.contacts &&
                f.flip_known && f.flip_id==-1 && f.selection_object==mode_gap_selection_;
            mode_gap_selection_=0;
            if(!resumed) {cancel();previous_={};return;}
        }
        if(phase_==VertPhase::released && suspended_) {
            // A busy publisher withdraws authority, not the physical impulse
            // already delivered. Resume only from bounded original-step proof;
            // a collision or changed cadence cannot borrow the old prediction.
            const float elapsed=static_cast<float>(f.physics_step-previous_.native.physics_step)*previous_.native.physics_seconds;
            auto expected=previous_.velocity;expected[1]-=gravity_*elapsed;
            const auto delta=sub(value.velocity,expected);
            if(!same_flight(previous_.native) || f.sampled_at-previous_.native.sampled_at>100 || elapsed>.1f ||
               std::abs(f.physics_seconds-previous_.native.physics_seconds)>.00001f ||
               (f.airborne && (!same_flight(f) || std::hypot(delta[0],delta[2])>.3f || std::abs(delta[1])>.5f ||
                length(sub(value.position,position(previous_.position,previous_.velocity,gravity_,elapsed)))>.25f)))cancel();
            suspended_=false;
        }
        landing_.observe(value);
        if(!f.airborne) {
            // Only a real supported riding observation records landing. A query
            // destination or a state departure on its own cannot do so.
            const bool landed=same_flight(previous_.native) && value.supported &&
                (f.state==100 || f.state==103) && (phase_==VertPhase::released || phase_==VertPhase::native_owned) &&
                (released_plan_.action==VertAction::level_out ||
                 (length(sub(value.position,released_plan_.landing))<=1.5f &&
                  dot(value.up,released_plan_.normal)>.7f &&
                  std::any_of(released_plan_.destination.samples.begin(),released_plan_.destination.samples.end(),
                    [&](const auto& sample){return sample.body==value.contacted_shape;})));
            request_=0;plan_={};gravity_=0;gravity_samples_=0;
            const auto ramp=value.ramp();const auto n=ramp.normal;
            const auto before=previous_.ramp();
            // A steep plane alone is not a transition. Observe the concave
            // change of support normal while physically climbing the ramp.
            if(ramp.valid && value.supported && before.valid && previous_.supported &&
               !previous_.native.airborne && f.sampled_at>=previous_.native.sampled_at &&
               f.sampled_at-previous_.native.sampled_at<=100 && n[1]<before.normal[1]-.005f &&
               dot(sub(n,before.normal),value.velocity)<-.05f &&
               value.position[1]>previous_.position[1]+.005f &&
               length(sub(value.position,previous_.position))<3)transition_at_=f.sampled_at;
            const bool curved=transition_at_ && f.sampled_at>=transition_at_ && f.sampled_at-transition_at_<=500;
            const bool steep=curved &&ramp.valid && value.supported && (f.state==100 || f.state==103) &&
                f.mode<=2 && n[1]>=-.01f && n[1]<.55f && value.velocity[1]>2 &&
                dot(value.up,n)>.7f && std::abs(dot(value.velocity,n))<2;
            if(steep) {origin_=ramp;lip_=value;phase_=VertPhase::rising_support;}
            else {
                // Wheel contact can end a few original steps before AIR is
                // published. Preserve that departure, not a new launch: its
                // deadline remains anchored to the last real curved support.
                // A native transition can retain only its rear wheel at the
                // lip. That real contact may continue an already proven curved
                // ramp, but a single contact can never recognize a new ramp.
                const bool retained=phase_==VertPhase::rising_support && curved && value.supported && !ramp.valid &&
                    lip_.native.valid && f.sampled_at>=lip_.native.sampled_at && f.sampled_at-lip_.native.sampled_at<=100 &&
                    f.owner==lip_.native.owner && f.owner_generation==lip_.native.owner_generation &&
                    (f.state==100 || f.state==103) && f.mode<=2 && !f.rail_family && value.velocity[1]>2 &&
                    finite(value.contact_point) && finite(value.contact_normal) &&
                    std::abs(dot(value.contact_normal,value.contact_normal)-1)<.02f &&
                    dot(value.contact_normal,origin_.normal)>.9f && dot(value.up,value.contact_normal)>.7f &&
                    std::abs(dot(value.velocity,value.contact_normal))<2 &&
                    length(sub(value.position,value.contact_point))<1.2f && length(sub(value.position,lip_.position))<2 &&
                    std::any_of(origin_.contacts.begin(),origin_.contacts.end(),[&](const auto& contact) {
                        return contact.shape==value.contacted_shape && contact.owner==value.contact_owner;
                    });
                if(retained) {lip_=value;origin_.point=value.contact_point;origin_.normal=value.contact_normal;}
                const bool departing=phase_==VertPhase::rising_support && (!value.supported || !ramp.valid) &&
                    lip_.native.valid && f.owner==lip_.native.owner && f.owner_generation==lip_.native.owner_generation &&
                    f.sampled_at>=lip_.native.sampled_at && f.sampled_at-lip_.native.sampled_at<=100 &&
                    (f.state==100 || f.state==103) && f.mode<=2 && !f.rail_family && value.velocity[1]>2 &&
                    dot(value.up,origin_.normal)>.7f && length(sub(value.position,lip_.position))<2;
                phase_=departing?VertPhase::rising_support:landed?VertPhase::landed:VertPhase::idle;
            }
        } else if(!previous_.native.airborne) {
            if(phase_==VertPhase::rising_support && previous_.native.valid &&
               f.airborne_started_known && lip_.native.valid && f.airborne_started_at>=lip_.native.sampled_at &&
               f.sampled_at>=lip_.native.sampled_at && f.sampled_at-lip_.native.sampled_at<=100 && value.velocity[1]>2 && !f.rail_family && f.mode<=2) {
                takeoff_=value;phase_=VertPhase::native_air;landing_.begin(value);
                direction_=scale(horizontal(origin_.normal),-1);
            } else cancel();
        } else if((phase_==VertPhase::native_air || phase_==VertPhase::requested || phase_==VertPhase::released ||
                 phase_==VertPhase::native_owned) &&
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
            if(request_ && now>=request_at_ && now-request_at_>150) {
                // An unavailable transfer expires its own command, not genuine
                // vert flight or the later independently proven landing window.
                // The shared journal still owns the original physical edge.
                request_=0;plan_={};
                if(phase_==VertPhase::requested && matches(f,now))phase_=VertPhase::native_air;
                else cancel();
            }
            return {};
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
    // Called only after the native AIR updater accepted this exact Level Out
    // packet and the transactional body writer completed. Submission transfers
    // future motion/alignment ownership, not evidence of curve adoption or a
    // landing. Native articulation is not a point-mass ballistic prediction.
    bool submitted_native_level_out(const NativeInputFrame& f,std::uint64_t now,const VertPlan& plan) noexcept {
        const auto& previous=previous_.native;
        if(phase_!=VertPhase::released || suspended_ || !matches(f,now) ||
           f.physics_step!=previous.physics_step || f.sampled_at!=previous.sampled_at ||
           f.action_generation!=previous.action_generation || f.contact_generation!=previous.contact_generation ||
           f.mode_generation!=previous.mode_generation || f.selection_object!=previous.selection_object ||
           f.stance_and_push!=previous.stance_and_push || f.state!=201 || f.contacts || f.mode>2 ||
           f.rail_family || !f.flip_known || f.flip_id!=-1 || now<released_at_ || now-released_at_>50 ||
           plan.action!=VertAction::level_out || released_plan_.action!=VertAction::level_out ||
           !plan || plan.destination.valid || plan.velocity!=released_plan_.velocity ||
           plan.normal!=released_plan_.normal)return false;
        phase_=VertPhase::native_owned;return true;
    }
    SurfaceGeometryVector alignment(const NativeInputFrame& f,std::uint64_t now) const noexcept {
        if(phase_!=VertPhase::released || suspended_ || !matches(f,now) || now<released_at_ || now-released_at_>2500 ||
           !f.flip_known || f.flip_id!=-1 || f.mode>2)return {};
        return released_plan_.normal;
    }
    bool landing_contact_pending() const noexcept {return landing_gap_frame_.valid;}
    VertLandingWindow& landing() noexcept {return landing_;}
    const VertLandingWindow& landing() const noexcept {return landing_;}
    const VertObservation& observation() const noexcept {return previous_;}
    const VertRampSupport& origin() const noexcept {return origin_;}
    const VertPlan& released_plan() const noexcept {return released_plan_;}
    SurfaceGeometryVector direction() const noexcept {return direction_;}
    float gravity() const noexcept {return gravity_samples_>=2?gravity_:0;}
    VertPhase phase() const noexcept {return phase_;}
private:
    VertPhase phase_{};
    bool suspended_{};
    VertLandingWindow landing_{};
    VertObservation previous_{},takeoff_{},lip_{};
    std::uintptr_t mode_gap_selection_{},push_gap_selection_{};
    std::uint32_t push_gap_stance_{};
    VertRampSupport origin_{};
    SurfaceGeometryVector direction_{};
    VertPlan plan_{},released_plan_{};
    std::uint64_t released_at_{},unavailable_at_{};
    NativeInputFrame planned_{},landing_gap_frame_{},publication_frame_{};
    NativeRequestSample publication_live_{};
    static bool finite_motion(const VertObservation& o) noexcept {
        return vert_math::finite(o.position) && vert_math::finite(o.velocity);
    }
    SurfaceGeometryVector planned_velocity_{},planned_position_{};
    std::uint64_t request_{},request_at_{},transition_at_{};
    float gravity_{};
    unsigned gravity_samples_{};
};
}
