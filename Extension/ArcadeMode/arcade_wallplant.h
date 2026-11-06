#pragma once
#include "arcade_air_motion.h"
#include "arcade_surface_geometry.h"

namespace dingosdk::arcade_mode {
// A real riding collision supplies this record. SurfaceGeometrySample (a ray
// query) deliberately cannot be converted into a collision record here.
struct WallplantContact {
    NativeInputFrame native{};
    SurfaceGeometryContext geometry{};
    SurfaceGeometryBody body{};
    SurfaceGeometryVector point{}, normal{}, approach_velocity{};
    std::uint64_t identity{}, sampled_at{};
    bool collision{}, static_world{}, rail{}, skater{};
};
struct WallplantPress {
    InputNativeContext native{};
    std::uint64_t sequence{}, pressed_at{};
    bool fresh_a{};
};
struct WallplantFrame {
    NativeInputFrame native{};
    SurfaceGeometryContext geometry{};
    SurfaceGeometryVector velocity{};
    std::uint64_t now{};
    float ground_clearance{};
    bool enabled{}, focused{}, controller_owned{}, competing_owner{};
    // Independent native observations, not inferred from an A request or ray.
    bool takeoff_observed{}, clearance_known{}, separated{};
};
struct WallplantParameters {
    float minimum_approach_sine{}, normal_speed_loss{}, minimum_exit_speed{}, vertical_exit_speed{};
    float maximum_speed{}, maximum_vertical_speed{}, minimum_height{}, maximum_normal_y{};
    std::uint64_t repeat_ms{}, contact_lease_ms{};
    // classic arcade reference Physics_Wallplant_* values are prototype tuning, not Skate
    // hardware acceptance. Preserve tangent instead of classic arcade's scalar speed loss.
    static WallplantParameters reference_prototype(GameplayProfile profile = {}) noexcept {
        constexpr float inches = GameplayProfile::metres_per_inch;
        return {std::sin(20.0f*3.14159265358979323846f/180.0f),225*inches,200*inches,
            500*inches,profile.hard_speed(),profile.launch_speed(200,LaunchDescriptor::ordinary()),
            24*inches,.1f,1000,InputTiming::native_sample_lease_ms};
    }
    bool valid() const noexcept {
        const std::array values{minimum_approach_sine,normal_speed_loss,minimum_exit_speed,
            vertical_exit_speed,maximum_speed,maximum_vertical_speed,minimum_height,maximum_normal_y};
        for(float v:values) if(!std::isfinite(v) || v<0 || v>=1000)return false;
        return minimum_approach_sine>0 && minimum_approach_sine<1 && minimum_exit_speed>0 &&
            vertical_exit_speed>0 && maximum_speed>minimum_exit_speed && maximum_vertical_speed>0 &&
            maximum_normal_y<.25f && repeat_ms>0 && contact_lease_ms>0 &&
            contact_lease_ms<=InputTiming::native_sample_lease_ms;
    }
};
struct WallplantCommand {
    AirMotionOutput motion{};
    NativeInputFrame native{};
    SurfaceGeometryContext geometry{};
    SurfaceGeometryBody body{};
    SurfaceGeometryVector point{}, normal{}, before{}, after{};
    std::uint64_t contact{}, input_sequence{}, issued_at{};
    explicit operator bool() const noexcept { return motion.ready; }
};
// Admission and a single velocity transaction only. This owns no animation,
// combo settlement, score credit, airborne state, or subsequent physics step.
class WallplantPolicy {
public:
    explicit WallplantPolicy(WallplantParameters parameters = WallplantParameters::reference_prototype()) noexcept
        : parameters_(parameters) {}
    // Reset only with the existing physical journal/session reset. Ordinary
    // cancellation retains the edge watermark so an old A cannot be replayed.
    void reset() noexcept { *this=WallplantPolicy(parameters_); }
    void cancel() noexcept { separated_=false; }
    WallplantCommand step(const WallplantFrame& frame,const WallplantContact& contact,
                          const WallplantPress& press) noexcept {
        const bool fresh_edge=press.fresh_a && press.sequence && press.sequence>seen_input_;
        if(press.sequence>seen_input_)seen_input_=press.sequence;
        if(!frame_ready(frame) || !parameters_.valid()) { cancel(); return {}; }
        if(owner_!=frame.native.owner || generation_!=frame.native.owner_generation) {
            owner_=frame.native.owner; generation_=frame.native.owner_generation;
            used_=false; separated_=false;
        }
        if(used_ && frame.separated && frame.native.physics_step>used_step_)separated_=true;
        if(!fresh_edge || !press.native.required || !press.native.fresh(frame.now) ||
           !press.native.airborne || press.native.rail_family ||
           press.native.generations.owner!=frame.native.owner_generation ||
           press.native.generations.contact!=frame.native.contact_generation ||
           press.native.generations.action!=frame.native.action_generation ||
           press.native.air_episode()!=InputAirEpisode{frame.native.airborne_episode,frame.native.air_departure} ||
           press.native.physics_step>frame.native.physics_step || press.pressed_at>frame.now ||
           frame.now-press.pressed_at>parameters_.contact_lease_ms ||
           press.pressed_at<frame.native.airborne_started_at ||
           !contact_ready(frame,contact))return {};
        if(used_ && (!separated_ || contact.identity==used_contact_ || frame.now<used_at_ ||
                    frame.now-used_at_<parameters_.repeat_ms || contact.sampled_at<=used_at_))return {};

        auto normal=contact.normal;
        const float length=std::hypot(normal[0],normal[1],normal[2]);
        if(!std::isfinite(length) || std::abs(length-1)> .01f)return {};
        for(float& v:normal)v/=length;
        if(std::abs(normal[1])>parameters_.maximum_normal_y)return {};
        // Reflect about the horizontal wall normal; the independently bounded
        // upward kick must not turn a slightly sloping wall into speed gain.
        normal[1]=0;
        const float horizontal_length=std::hypot(normal[0],normal[2]);
        normal[0]/=horizontal_length;normal[2]/=horizontal_length;
        const auto dot=[&](SurfaceGeometryVector v){return v[0]*normal[0]+v[2]*normal[2];};
        const float incoming=-dot(contact.approach_velocity);
        const float approach_speed=std::hypot(contact.approach_velocity[0],contact.approach_velocity[2]);
        if(!(incoming>0) || !(approach_speed>0) ||
           incoming/approach_speed<parameters_.minimum_approach_sine || dot(frame.velocity)>0)return {};
        // The original collision may have stopped inward root velocity before
        // A is processed. Its fresh pre-impact approach remains necessary;
        // already outward travel is rejected without response provenance.
        const float current_incoming=-dot(frame.velocity);
        const SurfaceGeometryVector tangent{frame.velocity[0]+current_incoming*normal[0],0,
                                             frame.velocity[2]+current_incoming*normal[2]};
        const float tangent_speed=std::hypot(tangent[0],tangent[2]);
        if(tangent_speed>=parameters_.maximum_speed)return {};
        const float available=std::sqrt(parameters_.maximum_speed*parameters_.maximum_speed-tangent_speed*tangent_speed);
        const float exit=std::min(available,std::max(parameters_.minimum_exit_speed,
                                                  incoming-parameters_.normal_speed_loss));
        WallplantCommand result{};
        result.native=frame.native;result.geometry=frame.geometry;result.body=contact.body;
        result.point=contact.point;result.normal=normal;result.before=frame.velocity;
        result.after={tangent[0]+exit*normal[0],
            std::min(parameters_.vertical_exit_speed,parameters_.maximum_vertical_speed),tangent[2]+exit*normal[2]};
        if(!finite_velocity(result.after))return {};
        result.motion.ready=true;result.motion.vertical=true;result.motion.horizontal=true;
        result.motion.velocity_y=result.after[1];result.motion.delta_x=result.after[0]-frame.velocity[0];
        result.motion.delta_z=result.after[2]-frame.velocity[2];
        result.contact=contact.identity;result.input_sequence=press.sequence;result.issued_at=frame.now;
        used_=true;separated_=false;used_contact_=contact.identity;used_at_=frame.now;used_step_=frame.native.physics_step;
        return result;
    }
    // Recheck immediately before the atomic native-body commit. A caller must
    // discard the command after one attempt, even if native validation refuses.
    bool current(const WallplantCommand& command,const WallplantFrame& frame,
                 const WallplantContact& contact) const noexcept {
        return command && frame_ready(frame) && contact_ready(frame,contact) &&
            command.issued_at==frame.now && command.native.owner==frame.native.owner &&
            command.native.owner_generation==frame.native.owner_generation &&
            command.native.physics_step==frame.native.physics_step &&
            command.geometry==frame.geometry && command.body==contact.body &&
            command.contact==contact.identity && command.before==frame.velocity;
    }
    template<std::size_t N>
    static bool apply_bodies(const WallplantCommand& command,
            std::array<SurfaceGeometryVector,N>& velocities,std::array<SurfaceGeometryVector,N>& spins) noexcept {
        if(!command || velocities[0]!=command.before)return false;
        return AirMotion::apply_bodies(command.motion,velocities,spins);
    }
private:
    static bool finite_velocity(SurfaceGeometryVector v) noexcept {
        for(float x:v)if(!std::isfinite(x) || std::abs(x)>=1000)return false;
        return true;
    }
    bool frame_ready(const WallplantFrame& f) const noexcept {
        return f.enabled && f.focused && f.controller_owned && !f.competing_owner &&
            f.native.valid && f.native.owner.valid() && f.native.owner_generation && f.native.physics_step &&
            f.native.airborne && !f.native.rail_family && f.native.airborne_episode &&
            f.native.airborne_started_known && f.takeoff_observed && f.clearance_known &&
            f.now>=f.native.sampled_at && f.now-f.native.sampled_at<=parameters_.contact_lease_ms &&
            f.geometry.fresh(f.now) && f.geometry.native.generations.owner==f.native.owner_generation &&
            f.geometry.owner==std::array<std::uintptr_t,5>{f.native.owner.base,f.native.owner.entity,
                f.native.owner.core,f.native.owner.context,f.native.owner.rig} &&
            f.geometry.native.airborne && !f.geometry.native.rail_family &&
            f.geometry.native.generations.contact==f.native.contact_generation &&
            f.geometry.native.generations.action==f.native.action_generation &&
            f.geometry.native.physics_step==f.native.physics_step &&
            f.geometry.native.air_episode()==InputAirEpisode{f.native.airborne_episode,f.native.air_departure} &&
            std::isfinite(f.ground_clearance) && f.ground_clearance>=parameters_.minimum_height && finite_velocity(f.velocity);
    }
    bool contact_ready(const WallplantFrame& f,const WallplantContact& c) const noexcept {
        return c.collision && c.static_world && !c.rail && !c.skater && c.identity &&
            c.body.world==f.geometry.world && c.body.index!=UINT32_MAX &&
            same_geometry(c.geometry,f.geometry) && c.geometry.native.fresh(f.now) &&
            c.native.valid && c.native.owner==f.native.owner &&
            c.native.owner_generation==f.native.owner_generation && c.native.airborne &&
            c.native.airborne_episode==f.native.airborne_episode && c.native.air_departure==f.native.air_departure &&
            c.native.physics_step && c.native.physics_step<=f.native.physics_step &&
            f.now>=c.native.sampled_at && f.now-c.native.sampled_at<=parameters_.contact_lease_ms &&
            c.sampled_at>=f.native.airborne_started_at && f.now>=c.sampled_at &&
            f.now-c.sampled_at<=parameters_.contact_lease_ms &&
            surface_geometry_detail::finite(c.point) && finite_velocity(c.normal) && finite_velocity(c.approach_velocity);
    }
    static bool same_geometry(const SurfaceGeometryContext& a,const SurfaceGeometryContext& b) noexcept {
        return a.owner==b.owner && a.client_context==b.client_context && a.world==b.world &&
            a.world_generation==b.world_generation && a.self_entity==b.self_entity &&
            a.self_generation==b.self_generation && a.phase==b.phase &&
            a.native.generations.owner==b.native.generations.owner && a.native.air_episode()==b.native.air_episode();
    }
    WallplantParameters parameters_;
    NativeInputOwner owner_{};
    std::uint64_t generation_{},seen_input_{},used_contact_{},used_at_{},used_step_{};
    bool used_{},separated_{};
};
}
