#pragma once
#include "arcade_air_motion.h"
#include "arcade_surface_geometry.h"

namespace dingosdk::arcade_mode {
// Keep request provenance and current execution inputs separate. The native
// input importer can publish wheel contact/mode before the chooser changes
// selected AIR201. This is not a new physics observation or a landing receipt.
struct WallplantExecutionProof {
    NativeInputFrame native{};
    NativeRequestSample live{};
    bool imported_contact{};
};
inline bool wallplant_imported_contact_phase(const NativeInputCorrelation& correlation,
        const NativeRequestSample& live) noexcept {
    const auto& original=correlation.observed;
    const auto contacts=live.words[1]&0xc000u;
    // 1009c40 imports mode; 47d7ac0 imports the two wheel pairs, swapping
    // them by stance. 929 HIL measured exactly mode2->1 and contact0->8000
    // while selected state remained201. Both axle identities use this source
    // path; both-axle contact and every additional mismatch remain rejected.
    return correlation.mismatches==(NativeInputCorrelation::mode_changed|NativeInputCorrelation::contacts_changed) &&
        original.valid && original.airborne && original.owner.valid() && original.state==201 &&
        original.mode==2 && original.contacts==0 && live.valid && live.state==201 &&
        live.crouch_mode==1 && (contacts==0x4000u || contacts==0x8000u);
}
inline bool wallplant_execution_current(const WallplantExecutionProof& proof,const NativeInputOwner& owner,
        const NativeRequestSample& live,std::uint64_t now) noexcept {
    return proof.native.valid && proof.native.airborne && owner.valid() && proof.native.owner==owner &&
        now>=proof.native.sampled_at && now-proof.native.sampled_at<=InputTiming::native_sample_lease_ms &&
        proof.live.valid && proof.live.state==proof.native.state && live.valid &&
        live.state==proof.live.state && live.crouch_mode==proof.live.crouch_mode &&
        (live.words[0]&0x60u)==(proof.live.words[0]&0x60u) &&
        (live.words[1]&0xc000u)==(proof.live.words[1]&0xc000u);
}
// The existing native chooser receives the current state before its caller
// commits the returned state. This proof admits only the already-observed AIR
// owner at that seam; it never supplies or overrides the next native state.
inline bool wallplant_selection_current(const NativeInputFrame& frame,
        const NativeInputOwner& owner,std::uintptr_t selector,std::uintptr_t core_selector,
        std::uintptr_t selector_context,std::uint32_t current,std::uint32_t selected,
        std::uint64_t now) noexcept {
    return frame.valid && frame.airborne && frame.owner.valid() && frame.owner==owner &&
        selector && selector==core_selector && selector_context==owner.context &&
        (current==200 || current==201) && selected==current && frame.state==current &&
        now>=frame.sampled_at && now-frame.sampled_at<=InputTiming::native_sample_lease_ms;
}
// Prepared only inside an admitted AIR chooser invocation. This records the
// original result; it never supplies a selected state. Taking it removes its
// authority before a native callback can reenter the writer.
struct WallplantSelectionTicket {
    std::uint64_t invocation{}, input{};
    std::uintptr_t selector{};
    std::uint32_t before{}, chosen{};
    bool selected{};
    bool finish(std::uint64_t call,std::uintptr_t current_selector,std::uint32_t current,
            std::uint32_t result) noexcept {
        if(!invocation || call!=invocation || selector!=current_selector || before!=current ||
           (result!=before && !(before==201 && result==100))) { *this={};return false; }
        chosen=result;selected=true;return true;
    }
    WallplantSelectionTicket take() noexcept { auto result=*this;*this={};return result; }
};
inline bool wallplant_same_observation(const NativeInputFrame& a,const NativeInputFrame& b) noexcept {
    return a.valid && b.valid && a.owner==b.owner && a.owner_generation==b.owner_generation &&
        a.contact_generation==b.contact_generation && a.action_generation==b.action_generation &&
        a.physics_step==b.physics_step && a.sampled_at==b.sampled_at && a.state==b.state &&
        a.airborne && b.airborne && a.airborne_episode==b.airborne_episode && a.air_departure==b.air_departure;
}
// A retained collision only scopes arbitration with an in-progress input
// dispatch. Read pending AFTER that serialized dispatch completes; neither a
// retained collision nor the synchronized frame can manufacture an A request.
template<class Probe,class Synchronize,class Pending>
Probe wallplant_pending_after_arbitration(const Probe& retained,WallplantExecutionProof& execution,
        Synchronize&& synchronize,Pending&& pending) {
    if(!retained.contact || !retained.raw || !retained.native.valid)return {};
    execution=synchronize();
    if(!wallplant_same_observation(retained.native,execution.native))return {};
    const auto request=pending();
    if(!request.input || request.contact!=retained.contact || request.raw!=retained.raw ||
       !wallplant_same_observation(execution.native,request.native))return {};
    return request;
}
inline bool wallplant_reconstruction_current(const WallplantSelectionTicket& ticket,
        const WallplantExecutionProof& proof,const NativeInputOwner& owner,std::uintptr_t selector,
        std::uint32_t previous,const NativeRequestSample& live,std::uint64_t now) noexcept {
    if(!ticket.invocation || !ticket.input || !ticket.selected || !selector || ticket.selector!=selector ||
       (ticket.before!=200 && ticket.before!=201) || !proof.live.valid || proof.live.state!=ticket.before ||
       ticket.before!=proof.native.state || live.state!=ticket.chosen || !proof.native.valid ||
       !proof.native.airborne || !owner.valid() || owner!=proof.native.owner || !live.valid ||
       now<proof.native.sampled_at || now-proof.native.sampled_at>InputTiming::native_sample_lease_ms ||
       live.crouch_mode!=proof.live.crouch_mode || (live.words[0]&0x60u)!=(proof.live.words[0]&0x60u) ||
       (live.words[1]&0xc000u)!=(proof.live.words[1]&0xc000u))return false;
    if(ticket.chosen==ticket.before)return wallplant_execution_current(proof,owner,live,now);
    // 47d3fa0 stores the old selected state in +1410 before native100 entry
    // 482c200 reconstructs velocities. Corroborate this exact chooser result;
    // the previous-state field never authorizes a ground request by itself.
    return proof.imported_contact && ticket.before==201 && ticket.chosen==100 && previous==201;
}
// Original collision resolution may already produce outward normal velocity.
// Only the recorded AIR201 -> native100 reconstruction can corroborate that
// response; a generic outward frame remains ineligible for a new kick.
struct WallplantReconstructionProof {
    WallplantSelectionTicket ticket{};
    WallplantExecutionProof execution{};
    NativeRequestSample live{};
    std::uintptr_t selector{};
    std::uint32_t previous{};
    std::uint64_t contact{};
    bool matches(const NativeInputFrame& frame,std::uint64_t input,std::uint64_t identity,
            std::uint64_t now) const noexcept {
        return input && ticket.input==input && contact==identity && contact &&
            ticket.before==201 && ticket.chosen==100 && execution.imported_contact &&
            wallplant_same_observation(execution.native,frame) &&
            wallplant_reconstruction_current(ticket,execution,frame.owner,selector,previous,live,now);
    }
};
struct WallplantPolicyDecision {
    const char* reason{"not_evaluated"};
    float current_normal{},target_normal{};
    bool reconstruction{};
    bool collision_response{};
};
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
// Supplied by the exact collision producer after its original solver ran.
// Runtime joins raw to the active native invocation before using this proof;
// it is distinct from a chooser result and never claims reconstruction100.
struct WallplantCollisionResponseProof {
    WallplantExecutionProof execution{};
    NativeRequestSample live{};
    std::uint64_t input{},contact{},raw{},sampled_at{};
    bool matches(const NativeInputFrame& frame,std::uint64_t sequence,
                 const WallplantContact& hit,std::uint64_t now) const noexcept {
        return raw && input && input==sequence && contact && contact==hit.identity &&
            sampled_at==hit.sampled_at && sampled_at>=frame.sampled_at && sampled_at<=now &&
            wallplant_same_observation(execution.native,frame) &&
            (live.state==200 || live.state==201) &&
            wallplant_execution_current(execution,frame.owner,live,now);
    }
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
    float clearance_launch_speed{}, minimum_height{}, maximum_normal_y{};
    std::uint64_t repeat_ms{}, contact_lease_ms{};
    // Full horizontal reflection followed by scalar speed loss. Ordinary
    // riding/launch profile caps do not alter this collision exit trajectory.
    static WallplantParameters reference_prototype(GameplayProfile profile = {}) noexcept {
        constexpr float inches = GameplayProfile::metres_per_inch;
        return {std::sin(20.0f*3.14159265358979323846f/180.0f),225*inches,200*inches,
            500*inches,profile.launch_speed(200,LaunchDescriptor::ordinary()),
            24*inches,.1f,1000,InputTiming::native_sample_lease_ms};
    }
    bool valid() const noexcept {
        const std::array values{minimum_approach_sine,normal_speed_loss,minimum_exit_speed,
            vertical_exit_speed,clearance_launch_speed,minimum_height,maximum_normal_y};
        for(float v:values) if(!std::isfinite(v) || v<0 || v>=1000)return false;
        return minimum_approach_sine>0 && minimum_approach_sine<1 && minimum_exit_speed>0 &&
            vertical_exit_speed>0 && clearance_launch_speed>0 &&
            maximum_normal_y<.25f && repeat_ms>0 && contact_lease_ms>0 &&
            contact_lease_ms<=InputTiming::native_sample_lease_ms;
    }
};
struct WallplantCommand {
    AirMotionOutput motion{};
    NativeInputFrame native{};
    SurfaceGeometryContext geometry{};
    SurfaceGeometryBody body{};
    SurfaceGeometryVector point{}, normal{}, contact_normal{}, before{}, after{};
    SurfaceGeometryVector incoming{}, reflected{}, hold_forward{}, anchor{};
    bool anchor_verified{};
    std::uint64_t contact{}, input_sequence{}, issued_at{};
    explicit operator bool() const noexcept { return motion.ready; }
};
// Read-only clearance query used only after an independently observed airborne
// wall collision. Reach covers the existing maximum ordinary launch apex plus
// The reference minimum plant height; it does not enlarge ordinary support probes.
inline SurfaceGeometryRequest wallplant_clearance_request(SurfaceGeometryVector root,
        WallplantParameters parameters=WallplantParameters::reference_prototype()) noexcept {
    const float reach=std::min(19.25f,parameters.clearance_launch_speed*parameters.clearance_launch_speed/
        (2*AirMotion::ordinary_gravity)+parameters.minimum_height);
    constexpr std::array<std::array<float,2>,5> offsets{{{0,0},{-.25f,0},{.25f,0},{0,-.25f},{0,.25f}}};
    SurfaceGeometryRequest request{};
    for(std::size_t i=0;i<offsets.size();++i)
        request.segments[i]={{root[0]+offsets[i][0],root[1]+.75f,root[2]+offsets[i][1]},
            {root[0]+offsets[i][0],root[1]-reach,root[2]+offsets[i][1]}};
    return request;
}
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
                          const WallplantPress& press,const WallplantReconstructionProof* reconstruction=nullptr,
                          WallplantPolicyDecision* diagnostic=nullptr,
                          const WallplantCollisionResponseProof* collision=nullptr) noexcept {
        if(diagnostic)*diagnostic={};
        const auto reject=[&](const char* reason) {
            if(diagnostic)diagnostic->reason=reason;
            return WallplantCommand{};
        };
        const bool fresh_edge=press.fresh_a && press.sequence && press.sequence>seen_input_;
        if(press.sequence>seen_input_)seen_input_=press.sequence;
        if(!frame_ready(frame) || !parameters_.valid()) { cancel(); return reject("policy_frame"); }
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
           !contact_ready(frame,contact))return reject("policy_press_contact");
        if(used_ && (!separated_ || contact.identity==used_contact_ || frame.now<used_at_ ||
                    frame.now-used_at_<parameters_.repeat_ms || contact.sampled_at<=used_at_))return reject("policy_repeat");

        auto normal=contact.normal;
        const float length=std::hypot(normal[0],normal[1],normal[2]);
        if(!std::isfinite(length) || std::abs(length-1)> .01f)return reject("policy_normal");
        for(float& v:normal)v/=length;
        if(std::abs(normal[1])>parameters_.maximum_normal_y)return reject("policy_wall_normal");
        // Reflect about the horizontal wall normal; the independently bounded
        // upward kick must not turn a slightly sloping wall into speed gain.
        normal[1]=0;
        const float horizontal_length=std::hypot(normal[0],normal[2]);
        normal[0]/=horizontal_length;normal[2]/=horizontal_length;
        const auto dot=[&](SurfaceGeometryVector v){return v[0]*normal[0]+v[2]*normal[2];};
        const float incoming=-dot(contact.approach_velocity);
        const float approach_speed=std::hypot(contact.approach_velocity[0],contact.approach_velocity[2]);
        const float current_normal=dot(frame.velocity);
        const bool reconstructed=reconstruction && reconstruction->matches(frame.native,press.sequence,contact.identity,frame.now);
        const bool collided=collision && collision->matches(frame.native,press.sequence,contact,frame.now);
        const bool response=reconstructed || collided;
        if(diagnostic){diagnostic->current_normal=current_normal;diagnostic->reconstruction=reconstructed;diagnostic->collision_response=collided;}
        if(std::hypot(contact.approach_velocity[0],contact.approach_velocity[1],contact.approach_velocity[2])<
               .01f*GameplayProfile::metres_per_inch || !(incoming>0) || !(approach_speed>0) ||
           incoming/approach_speed<parameters_.minimum_approach_sine)return reject("policy_approach");
        if(current_normal>0 && !response)return reject("policy_outward_unproven");
        // Reflect the independently recorded incoming vector, not the solver's
        // already-resolved velocity. The transaction below applies only the
        // difference from the current body velocity, so it cannot reflect twice.
        const SurfaceGeometryVector reflected{contact.approach_velocity[0]+2*incoming*normal[0],0,
                                               contact.approach_velocity[2]+2*incoming*normal[2]};
        const float reflected_speed=std::hypot(reflected[0],reflected[2]);
        const float exit_speed=std::max(parameters_.minimum_exit_speed,reflected_speed-parameters_.normal_speed_loss);
        constexpr float zero_speed=.0001f*GameplayProfile::metres_per_inch;
        const SurfaceGeometryVector direction=reflected_speed>zero_speed ?
            SurfaceGeometryVector{reflected[0]/reflected_speed,0,reflected[2]/reflected_speed} :
            SurfaceGeometryVector{-contact.approach_velocity[0]/approach_speed,0,-contact.approach_velocity[2]/approach_speed};
        const float exit_normal=exit_speed*dot(direction);
        if(diagnostic)diagnostic->target_normal=exit_normal;
        if(current_normal>0 && current_normal>=exit_normal)return reject("policy_exit_already_reached");
        WallplantCommand result{};
        result.native=frame.native;result.geometry=frame.geometry;result.body=contact.body;
        result.point=contact.point;result.normal=normal;result.contact_normal=contact.normal;result.before=frame.velocity;
        result.incoming=contact.approach_velocity;result.reflected=reflected;result.hold_forward=direction;
        result.after={direction[0]*exit_speed,parameters_.vertical_exit_speed,direction[2]*exit_speed};
        if(!finite_velocity(result.after))return reject("policy_result");
        result.motion.ready=true;result.motion.vertical=true;result.motion.horizontal=true;
        result.motion.velocity_y=result.after[1];result.motion.delta_x=result.after[0]-frame.velocity[0];
        result.motion.delta_z=result.after[2]-frame.velocity[2];
        result.contact=contact.identity;result.input_sequence=press.sequence;result.issued_at=frame.now;
        used_=true;separated_=false;used_contact_=contact.identity;used_at_=frame.now;used_step_=frame.native.physics_step;
        if(diagnostic)diagnostic->reason="command_ready";
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
