#include "arcade_wallplant_runtime.h"
#include "arcade_native_wall_contact.h"
#include "arcade_wallplant_animation.h"
#include "Engine/Core/Log/logging.h"
#include <Windows.h>
#include <atomic>
#include <mutex>

namespace dingosdk::arcade_mode::wallplant_runtime {
namespace {
using Raw=native_wall_contact::Snapshot;
using Hit=native_wall_contact::Contact;
struct Candidate {
    WallplantContact contact{};
    Hit hit{};
    SurfaceGeometrySample wall{},ground{};
    SurfaceGeometryVector root{};
    float clearance{};
    std::uint64_t published_at{};
    bool valid{};
};
std::mutex mutex;
WallplantPolicy policy;
NativeInputFrame observed{};
Raw raw{};
NativeInputFrame raw_frame{};
Candidate candidate{};
WallplantPress pending{};
WallplantCommand in_flight{},receipt{},play_pending{};
std::uint64_t contact_identity{},last_raw{},last_used_contact{},last_success_at{},last_handled_input{};
bool grounded{},takeoff{},separated{},contact_active{},permitted{},animation_ready{};
std::atomic<bool> interrupt{};
NoBailSkater skater(NativeInputOwner o) noexcept {return {o.base,o.entity,o.core,o.context,o.rig};}
InputNativeContext context(const NativeInputFrame& f) noexcept {
    InputNativeContext n{};n.required=true;n.valid=f.valid;
    n.generations={f.owner_generation,f.contact_generation,f.action_generation};
    n.physics_step=f.physics_step;n.sampled_at=f.sampled_at;n.rail_family=f.rail_family;
    n.airborne=f.airborne;n.airborne_episode=f.airborne_episode;n.air_departure=f.air_departure;
    return n;
}
bool flight(const NativeInputFrame& a,const NativeInputFrame& b) noexcept {
    return a.valid && b.valid && a.owner==b.owner && a.owner_generation==b.owner_generation &&
        a.airborne && b.airborne && a.airborne_episode && a.airborne_episode==b.airborne_episode &&
        a.air_departure==b.air_departure;
}
bool fresh(const NativeInputFrame& f,std::uint64_t now) noexcept {
    return f.valid && f.owner.valid() && f.owner_generation && f.physics_step &&
        now>=f.sampled_at && now-f.sampled_at<=InputTiming::native_sample_lease_ms;
}
bool raw_wall(const Hit& h) noexcept {
    // Mobility and full body/shape association exclude live skaters and moving
    // objects. Category bits are observed, never interpreted as guessed IDs.
    // The independent broad planar patch below rejects rails/poles/corners.
    return h.identity_valid && h.static_body && h.categories_known &&
        h.kind!=4 && h.kind!=5 && h.kind!=8 && h.kind!=11 &&
        std::isfinite(h.normal[1]) && std::abs(h.normal[1])<=.1f;
}
bool ready(std::uint64_t now,const NativeInputFrame& f) noexcept {
    if(!permitted || !animation_ready || !takeoff || !candidate.valid || pending.sequence || in_flight ||
       !fresh(f,now) || !flight(candidate.contact.native,f) || !flight(observed,f) ||
       now<candidate.published_at || now-candidate.published_at>InputTiming::native_sample_lease_ms ||
       now<candidate.contact.sampled_at || now-candidate.contact.sampled_at>InputTiming::native_sample_lease_ms ||
       now<candidate.wall.queried_at || now-candidate.wall.queried_at>InputTiming::native_sample_lease_ms ||
       now<candidate.ground.queried_at || now-candidate.ground.queried_at>InputTiming::native_sample_lease_ms)return false;
    if(last_used_contact && (candidate.contact.identity==last_used_contact || !separated ||
       now<last_success_at || now-last_success_at<WallplantParameters::reference_prototype().repeat_ms))return false;
    return native_wall_contact::current(candidate.hit,candidate.contact.geometry.world);
}
WallplantFrame frame(std::uint64_t now,const NativeInputFrame& f,SurfaceGeometryVector velocity) noexcept {
    WallplantFrame out{};out.native=f;out.geometry=candidate.contact.geometry;
    // Copy current physical identity without renewing either query timestamp.
    out.geometry.native=context(f);out.velocity=velocity;out.now=now;out.ground_clearance=candidate.clearance;
    out.enabled=permitted;out.focused=permitted;out.controller_owned=permitted;
    out.takeoff_observed=takeoff;out.clearance_known=candidate.valid;out.separated=separated;
    return out;
}
void withdraw() noexcept {
    candidate={};raw={};raw_frame={};pending={};in_flight={};play_pending={};
    takeoff=grounded=separated=contact_active=animation_ready=false;policy.cancel();
}
}

void cancel() noexcept {
    {std::lock_guard lock(mutex);permitted=false;withdraw();observed={};last_handled_input=0;receipt={};}
    interrupt.store(true,std::memory_order_release);
    native_wall_contact::enable(false);
}
void interrupt_animation() noexcept {interrupt.store(true,std::memory_order_release);}
void tick(std::uintptr_t base,bool gameplay) noexcept {
    const bool active=gameplay && native_wall_contact::start(base);
    native_wall_contact::enable(active);
    if(interrupt.exchange(false,std::memory_order_acq_rel)) {
        wallplant_animation::cancel();
        std::lock_guard lock(mutex);play_pending={};
    }
    wallplant_animation::tick(active);
    NativeInputFrame f{};WallplantCommand play{};
    {
        std::lock_guard lock(mutex);permitted=active;
        if(!active){withdraw();observed={};return;}
        f=observed;play=play_pending;play_pending={};
    }
    if(play) {
        const bool accepted=flight(play.native,f) && wallplant_animation::request(skater(play.native.owner),play.input_sequence);
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT VISUAL: occurrence={} requested={} approximation=true native_actor_verified=false native_scoring_verified=false.",
            play.input_sequence,accepted);
    }
    const bool available=fresh(f,GetTickCount64()) && f.airborne && wallplant_animation::available(skater(f.owner));
    std::lock_guard lock(mutex);animation_ready=available && flight(f,observed);
}
void observe_physics(const NativeInputFrame& f) noexcept {
    const auto contacts=f.valid ? native_wall_contact::observe(skater(f.owner),f.sampled_at) : Raw{};
    std::lock_guard lock(mutex);
    const bool same=observed.valid && f.valid && observed.owner==f.owner && observed.owner_generation==f.owner_generation &&
        f.physics_step>observed.physics_step && f.sampled_at>=observed.sampled_at &&
        f.sampled_at-observed.sampled_at<=InputTiming::native_sample_lease_ms;
    const auto preceding_at=observed.sampled_at;
    if(!same) {withdraw();last_used_contact=last_success_at=0;policy.reset();}
    if(!f.valid || !f.owner.valid()) {observed={};return;}
    if(!f.airborne) {
        withdraw();grounded=!f.rail_family && (f.state==100 || f.state==103);observed=f;
        interrupt.store(true,std::memory_order_release);return;
    }
    if(same && grounded && !observed.airborne)takeoff=true;
    grounded=false;observed=f;
    if(!permitted || !takeoff || !contacts.valid || contacts.serial==last_raw)return;
    last_raw=contacts.serial;
    if(contacts.sampled_at>f.sampled_at || contacts.sampled_at<preceding_at ||
       !std::isfinite(contacts.seconds) || contacts.seconds<=0 ||
       !AirMotion::same_velocity(contacts.seconds,f.physics_seconds))return;
    bool wall=false;
    if(contacts.state==200 || contacts.state==201) {
        for(std::size_t i=0;i<contacts.count;++i)wall=wall || raw_wall(contacts.contacts[i]);
    }
    if(!wall){separated=true;contact_active=false;return;}
    if(!contact_active) {
        if(contact_identity==UINT64_MAX){withdraw();return;}
        ++contact_identity;
    }
    contact_active=true;raw=contacts;raw_frame=f;
}
void publish_geometry(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
        const NativeInputFrame& f,SurfaceGeometryVector root,const SurfaceGeometrySample& ground) noexcept {
    Raw source{};NativeInputFrame origin{};std::uint64_t identity{};
    {
        std::lock_guard lock(mutex);candidate={};
        if(!permitted || !takeoff || !flight(raw_frame,f))return;
        source=raw;origin=raw_frame;identity=contact_identity;
    }
    if(!api.now || !api.current)return;
    const auto now=api.now(api.user);
    if(!source.valid || !identity || !fresh(f,now) || source.owner!=skater(f.owner) ||
       (source.state!=200 && source.state!=201) || now<source.sampled_at ||
       now-source.sampled_at>InputTiming::native_sample_lease_ms || !ctx.fresh(now) ||
       !ground.fresh_for(ctx,now) || !revalidate_surface_geometry(api,ctx,ground))return;
    const float clearance=root[1]-ground.point[1];
    if(!surface_geometry_detail::finite(root) || !std::isfinite(clearance) ||
       clearance<WallplantParameters::reference_prototype().minimum_height)return;
    for(std::size_t i=0;i<source.count;++i) {
        const auto& hit=source.contacts[i];
        if(!raw_wall(hit) || hit.other_entity==f.owner.entity || hit.shape.world!=ctx.world ||
           !native_wall_contact::current(hit,ctx.world))continue;
        const float approach=-(source.velocity[0]*hit.normal[0]+source.velocity[2]*hit.normal[2]);
        const float speed=std::hypot(source.velocity[0],source.velocity[2]);
        if(!(speed>0) || approach/speed<WallplantParameters::reference_prototype().minimum_approach_sine)continue;
        const SurfaceGeometryBody shape{hit.shape.world,hit.shape.index,hit.shape.generation};
        const auto wall=query_wall_geometry(api,ctx,shape,hit.point,hit.normal);
        if(!wall.valid || !native_wall_contact::current(hit,ctx.world) || !api.current(api.user,ctx))continue;
        Candidate result{};result.hit=hit;result.wall=wall;result.ground=ground;result.root=root;
        result.clearance=clearance;result.published_at=now;result.valid=true;
        auto& c=result.contact;c.native=origin;c.geometry=ctx;c.body=shape;c.point=hit.point;
        c.normal=hit.normal;c.approach_velocity=source.velocity;c.identity=identity;
        c.sampled_at=source.sampled_at;c.collision=c.static_world=true;
        std::lock_guard lock(mutex);
        if(permitted && takeoff && contact_identity==identity && flight(observed,f))candidate=result;
        return;
    }
}
InputSurfaceState surface(std::uint64_t now,const NativeInputFrame& f) noexcept {
    std::lock_guard lock(mutex);
    if(!ready(now,f))return {};
    return {candidate.contact.identity,f.physics_step,true,true,false,false,0,
        {f.owner_generation,f.contact_generation,f.action_generation}};
}
InputDisposition request(const InputRequest& request,std::uint64_t now,const NativeInputFrame& f) noexcept {
    std::lock_guard lock(mutex);
    if(request.action!=InputAction::wall_command || request.variant!=1 || !request.input_sequence ||
       request.input_sequence<=last_handled_input)return InputDisposition::cancelled;
    last_handled_input=request.input_sequence;
    if(!ready(now,f) || request.surface_generation!=candidate.contact.identity ||
       request.generations!=InputGenerations{f.owner_generation,f.contact_generation,f.action_generation} ||
       request.air_episode!=InputAirEpisode{f.airborne_episode,f.air_departure} ||
       now<request.physical_pressed_at || now-request.physical_pressed_at>InputTiming::native_sample_lease_ms)
        return InputDisposition::cancelled;
    pending={context(f),request.input_sequence,request.physical_pressed_at,true};
    return InputDisposition::started;
}
WallplantCommand consume(std::uint64_t now,const NativeInputFrame& f,SurfaceGeometryVector velocity) noexcept {
    std::lock_guard lock(mutex);
    if(!pending.sequence)return {};
    const auto press=pending;pending={};
    if(!ready(now,f))return {};
    auto current_frame=frame(now,f,velocity);
    in_flight=policy.step(current_frame,candidate.contact,press);
    return in_flight;
}
bool current(const WallplantCommand& command,std::uint64_t now,const NativeInputFrame& f,SurfaceGeometryVector velocity) noexcept {
    std::lock_guard lock(mutex);
    return permitted && candidate.valid && in_flight && command.input_sequence==in_flight.input_sequence &&
        flight(command.native,f) && fresh(f,now) &&
        native_wall_contact::current(candidate.hit,candidate.contact.geometry.world) &&
        policy.current(command,frame(now,f,velocity),candidate.contact);
}
void completed(const WallplantCommand& command,bool applied) noexcept {
    std::lock_guard lock(mutex);
    if(!command || !in_flight || command.input_sequence!=in_flight.input_sequence)return;
    in_flight={};
    if(!applied)return;
    receipt=command;play_pending=command;last_used_contact=command.contact;
    last_success_at=command.issued_at;separated=false;animation_ready=false;
    logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE WALLPLANT REBOUND: occurrence={} contact={} owner={} owner_generation={} episode={} departure={} step={} before={},{},{} after={},{},{} point={},{},{} normal={},{},{} applied=true native_scoring_verified=false combo_continuity_verified=false animation_verified=false.",
        command.input_sequence,command.contact,command.native.owner.core,command.native.owner_generation,
        command.native.airborne_episode,command.native.air_departure,command.native.physics_step,
        command.before[0],command.before[1],command.before[2],command.after[0],command.after[1],command.after[2],
        command.point[0],command.point[1],command.point[2],command.normal[0],command.normal[1],command.normal[2]);
}
WallplantCommand latest_completed() noexcept {std::lock_guard lock(mutex);return receipt;}
}
