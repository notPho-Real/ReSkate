#include "arcade_wallplant_runtime.h"
#include "arcade_native_wall_contact.h"
#include "arcade_wallplant_animation.h"
#include "arcade_wallplant_hold.h"
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
    SurfaceGeometryAnchor anchor{};
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
WallplantCommand in_flight{},receipt{};
WallplantHoldContinuity hold,combo_bridge;
WallplantCommand hold_play{},hold_stop{},grind_exclusion{};
Hit held_hit{};
std::uint64_t hold_play_started{},grind_exclusion_started{};
std::uint64_t contact_identity{},last_raw{},last_used_contact{},last_success_at{},last_handled_input{};
bool grounded{},takeoff{},separated{},contact_active{},permitted{},animation_ready{};
std::atomic<bool> interrupt{};
std::atomic<bool> stop_pose{};
std::atomic<std::uint64_t> pending_writer_input{};
std::atomic<bool> retained_contact_hint{};
// Diagnostic receipts only: bounded for the process lifetime and emitted once
// per stage/contact. This lock never acquires the runtime lock or calls native code.
std::mutex diagnostic_mutex;
struct DiagnosticSeen {std::uint64_t contact{},stages{};};
std::array<DiagnosticSeen,7> diagnostic_seen{};
unsigned diagnostic_count{};
unsigned query_diagnostic_count{};
DiagnosticSeen writer_seen{};
unsigned writer_diagnostic_count{};
DiagnosticSeen prepublish_seen{};
unsigned prepublish_count{};
DiagnosticSeen retained_writer_seen{};
unsigned retained_writer_count{};
struct RetainedPhaseRow {
    WriterProbe probe{};
    std::uint64_t now{},pending{};
    std::uintptr_t core{};
    unsigned stage{};
    const char* reason{};
    std::uint32_t state{};
    bool known{};
};
std::array<RetainedPhaseRow,128> retained_writer_rows{};
unsigned retained_writer_read{};
std::atomic<bool> retained_writer_queued{};
void flush_retained_phases() noexcept {
    if(!retained_writer_queued.load(std::memory_order_acquire))return;
    try {
        for(;;) {
            RetainedPhaseRow row{};
            {
                std::lock_guard lock(diagnostic_mutex);
                if(retained_writer_read==retained_writer_count) {
                    retained_writer_queued.store(false,std::memory_order_release);return;
                }
                row=retained_writer_rows[retained_writer_read++];
            }
            const auto& p=row.probe;
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE WALLPLANT PHASE: ms={} stage={} reason={} input_snapshot={} pending_now={} contact={} raw={} callback_core={} owner_generation={} step={} frame_ms={} frame_state={} state_known={} state={}.",
                row.now,row.stage,row.reason,p.input,row.pending,p.contact,p.raw,row.core,
                p.native.owner_generation,p.native.physics_step,p.native.sampled_at,
                p.native.state,row.known,row.state);
        }
    } catch(...) {}
}
void diagnose_query(const char* query,const SurfaceGeometryTrace& trace,const Raw& source,
        const NativeInputFrame& f,std::uint64_t identity) noexcept {
    try {
        std::lock_guard lock(diagnostic_mutex);
        if(query_diagnostic_count>=64)return;
        ++query_diagnostic_count;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT QUERY: query={} reason={} context_reason={} ms={} step={} state={} owner_generation={} contact={} raw={} ray={} attempt={} hits={} shape_world={} shape={} generation={} entity={} start={},{},{} end={},{},{} point={},{},{} value={}.",
            query,trace.reason,trace.context_reason,GetTickCount64(),f.physics_step,f.state,f.owner_generation,identity,source.serial,
            trace.ray,trace.attempt,trace.hits,trace.shape.world,trace.shape.index,trace.shape.generation,trace.entity,
            trace.start[0],trace.start[1],trace.start[2],trace.end[0],trace.end[1],trace.end[2],
            trace.point[0],trace.point[1],trace.point[2],trace.value);
    } catch(...) {}
}
void diagnose(unsigned phase,unsigned stage,const char* reason,const Raw& source,
        const NativeInputFrame& f,std::uint64_t now,std::uint64_t identity,
        std::uint64_t flags=0,float value=0,float extra=0,
        std::uint64_t input_sequence=0,std::uint64_t pressed_at=0) noexcept {
    if(!source.valid || !source.serial || phase>=diagnostic_seen.size() || stage>=64)return;
    try {
        std::lock_guard lock(diagnostic_mutex);
        if(diagnostic_count>=128)return;
        auto& seen=diagnostic_seen[phase];
        const auto key=identity?identity:source.serial;
        if(seen.contact!=key)seen={key,0};
        const auto bit=std::uint64_t{1}<<stage;
        if(seen.stages&bit)return;
        seen.stages|=bit;++diagnostic_count;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT GATE: phase={} stage={} reason={} ms={} step={} state={} airborne={} owner={} owner_generation={} episode={} departure={} contact={} raw={} source={} source_serial={} raw_ms={} age_valid={} age={} raw_state={} raw_after={} flags={} value={} extra={} input={} press_age_valid={} press_age={}.",
            phase,stage,reason,now,f.physics_step,f.state,f.airborne,f.owner.core,f.owner_generation,
            f.airborne_episode,f.air_departure,identity,source.serial,static_cast<unsigned>(source.source),
            source.source_serial,source.sampled_at,now>=source.sampled_at,
            now>=source.sampled_at?now-source.sampled_at:0,source.state,source.state_after,flags,value,extra,
            input_sequence,input_sequence&&now>=pressed_at,input_sequence&&now>=pressed_at?now-pressed_at:0);
    } catch(...) {} // Logging must never change the original physics callback.
}
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
bool same_command(const WallplantCommand& a,const WallplantCommand& b,bool anchored) noexcept {
    return a && b && a.input_sequence==b.input_sequence && a.contact==b.contact &&
        a.issued_at==b.issued_at && wallplant_same_observation(a.native,b.native) &&
        a.native.physics_seconds==b.native.physics_seconds && a.geometry==b.geometry && a.body==b.body &&
        a.point==b.point && a.normal==b.normal && a.contact_normal==b.contact_normal &&
        a.before==b.before && a.after==b.after && a.incoming==b.incoming &&
        a.reflected==b.reflected && a.hold_forward==b.hold_forward &&
        (!anchored || (a.anchor_verified && b.anchor_verified && a.anchor==b.anchor));
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
    if(!permitted || hold.active() || !animation_ready || !takeoff || !candidate.valid || pending.sequence || in_flight ||
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
void withdraw(const char* reason,const NativeInputFrame* frame=nullptr) noexcept {
    if(pending.sequence)diagnose(6,0,reason,raw,frame?*frame:observed,GetTickCount64(),contact_identity,
        0,0,0,pending.sequence,pending.pressed_at);
    retained_contact_hint.store(false,std::memory_order_release);
    candidate={};raw={};raw_frame={};pending={};in_flight={};
    if(hold.active())hold_stop=hold.command();
    hold.cancel(reason);combo_bridge.cancel(reason);hold_play={};held_hit={};
    pending_writer_input.store(0,std::memory_order_release);
    takeoff=grounded=separated=contact_active=animation_ready=false;policy.cancel();
}
}

void cancel() noexcept {
    {std::lock_guard lock(mutex);permitted=false;withdraw("cancel");observed={};last_handled_input=0;receipt={};}
    interrupt.store(true,std::memory_order_release);
    native_wall_contact::enable(false);
}
void interrupt_animation() noexcept {interrupt.store(true,std::memory_order_release);}
void tick(std::uintptr_t base,bool gameplay) noexcept {
    flush_retained_phases();
    const bool active=gameplay && native_wall_contact::start(base);
    native_wall_contact::enable(active);
    if(interrupt.exchange(false,std::memory_order_acq_rel)) {
        wallplant_animation::cancel();
        std::lock_guard lock(mutex);withdraw("controller_interrupt",&observed);
    }
    if(stop_pose.exchange(false,std::memory_order_acq_rel))wallplant_animation::cancel();
    wallplant_animation::tick(active);
    NativeInputFrame f{};WallplantCommand play{},stop{};std::uint64_t started{};
    {
        std::lock_guard lock(mutex);permitted=active;
        if(!active){withdraw("inactive");observed={};return;}
        f=observed;
        stop=hold_stop;hold_stop={};
        play=hold_play;hold_play={};started=hold_play_started;
        if(!hold.active() || hold.command().input_sequence!=play.input_sequence)play={};
    }
    if(stop)wallplant_animation::release(skater(stop.native.owner),stop.input_sequence);
    if(play) {
        bool accepted=!interrupt.load(std::memory_order_acquire) && !stop_pose.load(std::memory_order_acquire) &&
            wallplant_animation::begin_hold(skater(play.native.owner),play.input_sequence,started,play.point,play.contact_normal,play.hold_forward);
        bool still_held{};
        {std::lock_guard lock(mutex);still_held=hold.active() && same_command(hold.command(),play,true) &&
            hold.started_at()==started && hold.target_active(GetTickCount64());}
        if(accepted && (!still_held || interrupt.load(std::memory_order_acquire) || stop_pose.load(std::memory_order_acquire))) {
            wallplant_animation::cancel();accepted=false;
        }
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT VISUAL: occurrence={} requested={} approximation=true native_actor_verified=false native_scoring_verified=false.",
            play.input_sequence,accepted);
    }
    const bool available=fresh(f,GetTickCount64()) && f.airborne && wallplant_animation::available(skater(f.owner));
    std::lock_guard lock(mutex);animation_ready=available && flight(f,observed);
    const auto diagnostic_now=GetTickCount64();
    diagnose(2,animation_ready?1u:0u,"animation",raw,observed,diagnostic_now,contact_identity,
        (available?1ull:0ull)|(flight(f,observed)?2ull:0ull)|(animation_ready?4ull:0ull)|
        (fresh(f,diagnostic_now)?8ull:0ull)|(f.airborne?16ull:0ull));
}
void observe_physics(const NativeInputFrame& f) noexcept {
    const auto contacts=f.valid ? native_wall_contact::observe(skater(f.owner),f.sampled_at) : Raw{};
    std::lock_guard lock(mutex);
    const bool same=observed.valid && f.valid && observed.owner==f.owner && observed.owner_generation==f.owner_generation &&
        f.physics_step>observed.physics_step && f.sampled_at>=observed.sampled_at &&
        f.sampled_at-observed.sampled_at<=InputTiming::native_sample_lease_ms;
    const auto preceding_at=observed.sampled_at;
    bool wall=false;
    if(contacts.state==200 || contacts.state==201) {
        for(std::size_t i=0;i<contacts.count;++i)wall=wall || raw_wall(contacts.contacts[i]);
    }
    const auto retention=[&](unsigned stage,const char* reason) {
        if(wall)diagnose(0,stage,reason,contacts,f,f.sampled_at,contact_active?contact_identity:0,
            (same?1ull:0ull)|(permitted?2ull:0ull)|(takeoff?4ull:0ull)|
            (contacts.serial==last_raw?8ull:0ull),contacts.seconds,f.physics_seconds);
    };
    if(!same) {withdraw("physics_continuity",&f);last_used_contact=last_success_at=0;policy.reset();}
    if(!f.valid || !f.owner.valid()) {retention(0,"invalid_frame");observed={};return;}
    if(hold.active()) {
        const auto active_command=hold.command();
        if(permitted && hold.observe(f,GetTickCount64()) &&
           native_wall_contact::current(held_hit,hold.command().geometry.world)) {
            observed=f;return;
        }
        hold_stop=active_command;withdraw("hold_observation",&f);stop_pose.store(true,std::memory_order_release);
    }
    if(combo_bridge.active()) {
        if(!combo_bridge.observe(f,GetTickCount64()) || f.airborne)combo_bridge.cancel("released_air_or_invalid");
        else {observed=f;return;}
    }
    if(!f.airborne) {
        retention(1,"not_airborne");
        withdraw("physics_not_airborne",&f);grounded=!f.rail_family && (f.state==100 || f.state==103);observed=f;
        // Authenticated hold/release continuity returned above. Ordinary
        // ground observations still stop any owned pose.
        stop_pose.store(true,std::memory_order_release);return;
    }
    if(same && grounded && !observed.airborne)takeoff=true;
    grounded=false;observed=f;
    if(!permitted || !takeoff || !contacts.valid || contacts.serial==last_raw){retention(2,"retention_admission");return;}
    last_raw=contacts.serial;
    if(contacts.sampled_at>f.sampled_at){retention(3,"raw_after_frame");return;}
    if(contacts.sampled_at<preceding_at){retention(4,"raw_before_preceding_frame");return;}
    if(!std::isfinite(contacts.seconds) || contacts.seconds<=0 ||
       !AirMotion::same_velocity(contacts.seconds,f.physics_seconds)){retention(5,"physics_seconds");return;}
    if(!wall){separated=true;contact_active=false;retained_contact_hint.store(false,std::memory_order_release);return;}
    if(!contact_active) {
        if(contact_identity==UINT64_MAX){withdraw("contact_overflow",&f);return;}
        ++contact_identity;
    }
    contact_active=true;raw=contacts;raw_frame=f;
    retained_contact_hint.store(true,std::memory_order_release);
    retention(6,"retained");
}
void publish_geometry(const SurfaceGeometryNativeApi& supplied_api,const SurfaceGeometryContext& ctx,
        const NativeInputFrame& f,SurfaceGeometryVector root,const SurfaceGeometrySample& ground,
        std::span<const SurfaceGeometryVector> verified_support_offsets) noexcept {
    Raw source{};NativeInputFrame origin{};std::uint64_t identity{};
    {
        std::lock_guard lock(mutex);candidate={};
        if(!permitted || !takeoff || !flight(raw_frame,f)) {
            diagnose(1,0,"geometry_admission",raw,f,GetTickCount64(),contact_identity,
                (permitted?1ull:0ull)|(takeoff?2ull:0ull)|(flight(raw_frame,f)?4ull:0ull));return;
        }
        source=raw;origin=raw_frame;identity=contact_identity;
    }
    // Map collision owners are native world objects, not necessarily ECS
    // entities. Opt in only here, after real airborne collision retention.
    auto api=supplied_api;
    api.static_world_owner=[](void*,const SurfaceGeometryBody& shape,SurfaceGeometryWorldOwner& owner) noexcept {
        return native_wall_contact::static_world_owner(shape,owner);
    };
    api.local_component_owner=[](void*,const SurfaceGeometryContext& context,
            const SurfaceGeometryBody& shape,SurfaceGeometryWorldOwner& owner) noexcept {
        return native_wall_contact::local_component_owner(shape,context.owner[1],owner);
    };
    const auto geometry=[&](unsigned stage,const char* reason,std::uint64_t now,
            std::uint64_t flags=0,float value=0,float extra=0) {
        diagnose(1,stage,reason,source,f,now,identity,flags,value,extra);
    };
    if(verified_support_offsets.empty() || verified_support_offsets.size()>32) {
        geometry(15,"anchor_support_unavailable",GetTickCount64());return;
    }
    if(!api.now || !api.current){geometry(1,"geometry_api",GetTickCount64());return;}
    auto now=api.now(api.user);
    if(!source.valid || !identity || !fresh(f,now) || source.owner!=skater(f.owner) ||
       (source.state!=200 && source.state!=201) || now<source.sampled_at ||
       now-source.sampled_at>InputTiming::native_sample_lease_ms || !ctx.fresh(now)) {
        geometry(2,"geometry_provenance",now,(source.valid?1ull:0ull)|(identity?2ull:0ull)|
            (fresh(f,now)?4ull:0ull)|(source.owner==skater(f.owner)?8ull:0ull)|
            ((source.state==200||source.state==201)?16ull:0ull)|(ctx.fresh(now)?32ull:0ull));return;
    }
    auto clearance_ground=ground;
    const bool ground_reused=clearance_ground.fresh_for(ctx,now);
    if(!ground_reused) {
        SurfaceGeometryTrace trace;
        clearance_ground=query_surface_geometry(api,ctx,wallplant_clearance_request(root),&trace);
        if(!clearance_ground.valid)diagnose_query("clearance",trace,source,f,identity);
    }
    // A fallback stamps its own query time after the initial admission check.
    // Validate at the actual current clock, not against an earlier millisecond
    // which would misclassify a successful fresh query as a future observation.
    now=api.now(api.user);
    if(!clearance_ground.fresh_for(ctx,now)) {
        geometry(3,"ground_freshness",now,(clearance_ground.valid?1ull:0ull)|(ground_reused?2ull:0ull)|
            (clearance_ground.context.native.fresh(now)?4ull:0ull)|
            (clearance_ground.context.native.generations==ctx.native.generations?8ull:0ull)|
            (clearance_ground.context.native.air_episode()==ctx.native.air_episode()?16ull:0ull)|
            (now>=clearance_ground.queried_at&&now-clearance_ground.queried_at<=InputTiming::native_sample_lease_ms?32ull:0ull));return;
    }
    if(!revalidate_surface_geometry(api,ctx,clearance_ground)){geometry(4,"ground_current",now);return;}
    const float clearance=root[1]-clearance_ground.point[1];
    if(!surface_geometry_detail::finite(root) || !std::isfinite(clearance) ||
       clearance<WallplantParameters::reference_prototype().minimum_height){geometry(5,"ground_clearance",now,0,clearance,root[1]);return;}
    for(std::size_t i=0;i<source.count;++i) {
        const auto& hit=source.contacts[i];
        if(!raw_wall(hit) || hit.other_entity==f.owner.entity || hit.shape.world!=ctx.world) {
            geometry(6,"hit_identity",now,(raw_wall(hit)?1ull:0ull)|
                (hit.other_entity!=f.owner.entity?2ull:0ull)|(hit.shape.world==ctx.world?4ull:0ull));continue;
        }
        if(!native_wall_contact::current(hit,ctx.world)){geometry(7,"hit_current",now);continue;}
        const float approach=-(source.velocity[0]*hit.normal[0]+source.velocity[2]*hit.normal[2]);
        const float speed=std::hypot(source.velocity[0],source.velocity[2]);
        if(!(speed>0) || approach/speed<WallplantParameters::reference_prototype().minimum_approach_sine) {
            geometry(8,"approach",now,0,approach,speed);continue;
        }
        const SurfaceGeometryBody shape{hit.shape.world,hit.shape.index,hit.shape.generation};
        SurfaceGeometryTrace trace;
        const auto wall=query_wall_geometry(api,ctx,shape,hit.point,hit.normal,source.velocity,source.seconds,&trace);
        now=api.now(api.user);
        if(!wall.valid)diagnose_query("wall",trace,source,f,identity);
        if(!wall.valid){geometry(9,"broad_wall",now);continue;}
        SurfaceGeometryTrace anchor_trace;
        const auto anchor=query_wallplant_anchor(api,ctx,shape,hit.point,hit.normal,root,
            verified_support_offsets,&anchor_trace);
        if(!anchor.valid) {
            diagnose_query("anchor",anchor_trace,source,f,identity);
            geometry(16,"anchor_clearance",api.now(api.user));continue;
        }
        if(!native_wall_contact::current(hit,ctx.world)){geometry(10,"hit_current_after_query",now);continue;}
        if(!api.current(api.user,ctx)){geometry(11,"world_current_after_query",now);continue;}
        now=api.now(api.user);
        // Native queries/revalidation may advance time. Preserve every original
        // timestamp and reject expired contact/frame/geometry at publication.
        if(!fresh(f,now) || now<source.sampled_at ||
           now-source.sampled_at>InputTiming::native_sample_lease_ms ||
           !clearance_ground.fresh_for(ctx,now) || !wall.fresh_for(ctx,now)) {
            geometry(14,"publication_freshness",now);continue;
        }
        Candidate result{};result.hit=hit;result.wall=wall;result.ground=clearance_ground;result.root=root;
        result.anchor=anchor;
        result.clearance=clearance;result.published_at=now;result.valid=true;
        auto& c=result.contact;c.native=origin;c.geometry=ctx;c.body=shape;c.point=hit.point;
        c.normal=hit.normal;c.approach_velocity=source.velocity;c.identity=identity;
        c.sampled_at=source.sampled_at;c.collision=c.static_world=true;
        std::lock_guard lock(mutex);
        if(permitted && takeoff && contact_identity==identity && flight(observed,f))candidate=result;
        geometry(candidate.valid?12u:13u,candidate.valid?"geometry_published":"geometry_commit",now,
            (permitted?1ull:0ull)|(takeoff?2ull:0ull)|(contact_identity==identity?4ull:0ull)|
            (flight(observed,f)?8ull:0ull),clearance);
        return;
    }
}
InputSurfaceState surface(std::uint64_t now,const NativeInputFrame& f) noexcept {
    std::lock_guard lock(mutex);
    const bool eligible=ready(now,f);
    // Pure copies of the existing gates: no extra native identity queries.
    const std::uint64_t flags=(permitted?1ull:0ull)|(animation_ready?2ull:0ull)|(takeoff?4ull:0ull)|
        (candidate.valid?8ull:0ull)|(!pending.sequence?16ull:0ull)|(!in_flight?32ull:0ull)|
        (fresh(f,now)?64ull:0ull)|(flight(candidate.contact.native,f)?128ull:0ull)|
        (flight(observed,f)?256ull:0ull)|
        ((now>=candidate.published_at&&now-candidate.published_at<=InputTiming::native_sample_lease_ms)?512ull:0ull)|
        ((now>=candidate.contact.sampled_at&&now-candidate.contact.sampled_at<=InputTiming::native_sample_lease_ms)?1024ull:0ull)|
        ((now>=candidate.wall.queried_at&&now-candidate.wall.queried_at<=InputTiming::native_sample_lease_ms)?2048ull:0ull)|
        ((now>=candidate.ground.queried_at&&now-candidate.ground.queried_at<=InputTiming::native_sample_lease_ms)?4096ull:0ull)|
        ((!last_used_contact||(candidate.contact.identity!=last_used_contact&&separated&&
           now>=last_success_at&&now-last_success_at>=WallplantParameters::reference_prototype().repeat_ms))?8192ull:0ull);
    // Stages 0..13 identify the first missing flag above; 14 means the final
    // native target revalidation failed; 15 means all existing gates passed.
    unsigned gate=0;
    while(gate<14 && (flags&(std::uint64_t{1}<<gate)))++gate;
    diagnose(3,eligible?15u:gate,eligible?"surface_ready":"surface_rejected",raw,f,now,contact_identity,flags);
    if(!eligible)return {};
    return {candidate.contact.identity,f.physics_step,true,true,false,false,0,
        {f.owner_generation,f.contact_generation,f.action_generation}};
}
InputDisposition request(const InputRequest& request,std::uint64_t now,const NativeInputFrame& f) noexcept {
    std::lock_guard lock(mutex);
    const auto disposition=[&](unsigned stage,const char* reason) {
        diagnose(4,stage,reason,raw,f,now,contact_identity,0,0,0,request.input_sequence,request.physical_pressed_at);
    };
    if(request.action!=InputAction::wall_command || request.variant!=1 || !request.input_sequence ||
       request.input_sequence<=last_handled_input){disposition(0,"request_identity");return InputDisposition::cancelled;}
    last_handled_input=request.input_sequence;
    if(!ready(now,f)){disposition(1,"request_not_ready");return InputDisposition::cancelled;}
    if(request.surface_generation!=candidate.contact.identity ||
       request.generations!=InputGenerations{f.owner_generation,f.contact_generation,f.action_generation} ||
       request.air_episode!=InputAirEpisode{f.airborne_episode,f.air_departure} ||
       now<request.physical_pressed_at || now-request.physical_pressed_at>InputTiming::native_sample_lease_ms)
        {disposition(2,"request_provenance");return InputDisposition::cancelled;}
    pending={context(f),request.input_sequence,request.physical_pressed_at,true};
    pending_writer_input.store(pending.sequence,std::memory_order_release);
    disposition(3,"pending_accepted");
    return InputDisposition::started;
}
WallplantCommand consume(std::uint64_t now,const NativeInputFrame& f,SurfaceGeometryVector velocity,
        std::uint64_t expected_input,const WallplantReconstructionProof* reconstruction) noexcept {
    std::lock_guard lock(mutex);
    if(!pending.sequence)return {};
    // A selector handoff is valid only before another original physics
    // observation. Do not consume a replacement input or renew its AIR frame.
    if(expected_input && (pending.sequence!=expected_input || !wallplant_same_observation(f,observed)))return {};
    const auto press=pending;pending={};
    pending_writer_input.store(0,std::memory_order_release);
    const auto consumption=[&](unsigned stage,const char* reason) {
        diagnose(5,stage,reason,raw,f,now,contact_identity,0,0,0,press.sequence,press.pressed_at);
    };
    if(!ready(now,f)){consumption(0,"consume_not_ready");return {};}
    auto current_frame=frame(now,f,velocity);
    WallplantPolicyDecision decision{};
    in_flight=policy.step(current_frame,candidate.contact,press,reconstruction,&decision);
    if(in_flight) {
        in_flight.anchor=candidate.anchor.target;
        in_flight.anchor_verified=candidate.anchor.valid;
        if(!in_flight.anchor_verified)in_flight={};
    }
    diagnose(5,in_flight?2u:1u,decision.reason,raw,f,now,contact_identity,
        decision.reconstruction?1u:0u,decision.current_normal,decision.target_normal,press.sequence,press.pressed_at);
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
    // Legacy failure retirement only. Success requires the owned hold/release
    // transaction; a writer cannot bypass that phase by reporting completion.
    (void)applied;
}
bool begin_hold(const WallplantCommand& command,std::uint64_t& now) noexcept {
    std::lock_guard lock(mutex);
    now=GetTickCount64();
    if(!permitted || hold.active() || !command || !in_flight || !candidate.valid ||
       !command.anchor_verified || !surface_geometry_detail::finite(command.anchor) ||
       !same_command(command,in_flight,true) ||
       !native_wall_contact::current(candidate.hit,command.geometry.world) || !hold.begin(command,now))return false;
    held_hit=candidate.hit;in_flight={};hold_play=command;hold_play_started=now;
    combo_bridge.cancel();last_used_contact=command.contact;last_success_at=now;
    grind_exclusion=command;grind_exclusion_started=now;
    separated=false;animation_ready=false;return true;
}
bool hold_current(const WallplantCommand& command,std::uint64_t& now,const NativeInputFrame& live) noexcept {
    std::lock_guard lock(mutex);
    now=GetTickCount64();
    if(!hold.active() || !same_command(command,hold.command(),true))return false;
    if(permitted && native_wall_contact::current(held_hit,command.geometry.world) && hold.observe(live,now))return true;
    hold_stop=command;hold.cancel("hold_invalid");hold_play={};held_hit={};return false;
}
void finish_hold(const WallplantCommand& command,bool released,std::uint64_t now) noexcept {
    std::lock_guard lock(mutex);
    now=GetTickCount64();
    if(!hold.active() || !same_command(command,hold.command(),true))return;
    released=released && hold.release_due(now);
    if(released){receipt=hold.command();combo_bridge=hold;(void)combo_bridge.release_bridge(now);}
    hold_stop=hold.command();hold.cancel(released?"released":"cancelled");hold_play={};held_hit={};
    if(!released)return;
    logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE WALLPLANT REBOUND: occurrence={} contact={} owner={} owner_generation={} episode={} departure={} step={} before={},{},{} after={},{},{} point={},{},{} normal={},{},{} applied=true native_scoring_verified=false combo_continuity_verified=false animation_verified=false.",
        command.input_sequence,command.contact,command.native.owner.core,command.native.owner_generation,
        command.native.airborne_episode,command.native.air_departure,command.native.physics_step,
        command.before[0],command.before[1],command.before[2],command.after[0],command.after[1],command.after[2],
        command.point[0],command.point[1],command.point[2],command.normal[0],command.normal[1],command.normal[2]);
}
WallplantCommand held_command() noexcept {std::lock_guard lock(mutex);return hold.command();}
std::uint64_t hold_started_at() noexcept {std::lock_guard lock(mutex);return hold.started_at();}
bool preserves_combo(const NativeInputFrame& frame,std::uint64_t now) noexcept {
    std::lock_guard lock(mutex);
    now=GetTickCount64();
    auto proof=hold.active()?hold:combo_bridge;
    return permitted && proof.active() && proof.observe(frame,now) && frame.state==100 && !frame.airborne;
}
bool excludes_grind(const NativeInputFrame& frame,std::uint64_t now) noexcept {
    std::lock_guard lock(mutex);
    return permitted && grind_exclusion && frame.valid && frame.owner==grind_exclusion.native.owner &&
        frame.owner_generation==grind_exclusion.native.owner_generation && now>=grind_exclusion_started &&
        now-grind_exclusion_started<200;
}
WallplantCommand latest_completed() noexcept {std::lock_guard lock(mutex);return receipt;}
GeometryProbe geometry_probe() noexcept {
    if(!retained_contact_hint.load(std::memory_order_acquire))return {};
    std::lock_guard lock(mutex);
    if(!permitted || !takeoff || !contact_active || !raw.valid || !contact_identity || !fresh(raw_frame,GetTickCount64()))return {};
    return {contact_identity,raw.serial,raw_frame};
}
bool geometry_probe_current(const GeometryProbe& probe,const NativeInputFrame& frame) noexcept {
    std::lock_guard lock(mutex);
    const auto now=GetTickCount64();
    return permitted && takeoff && contact_active && raw.valid && probe.contact==contact_identity &&
        probe.raw==raw.serial && wallplant_same_observation(probe.native,raw_frame) &&
        fresh(raw_frame,now) && fresh(frame,now) && flight(raw_frame,frame);
}
void geometry_event(const GeometryProbe& probe,unsigned stage,const char* reason) noexcept {
    if(!probe.contact || !probe.raw || stage>=64)return;
    try {
        Raw source{};NativeInputFrame frame{};
        {
            std::lock_guard lock(mutex);
            if(probe.contact!=contact_identity || probe.raw!=raw.serial || !raw.valid)return;
            source=raw;frame=raw_frame;
        }
        std::lock_guard lock(diagnostic_mutex);
        if(prepublish_count>=128)return;
        if(prepublish_seen.contact!=probe.contact)prepublish_seen={probe.contact,0};
        const auto bit=std::uint64_t{1}<<stage;
        if(prepublish_seen.stages&bit)return;
        prepublish_seen.stages|=bit;++prepublish_count;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT PREPUBLISH: ms={} stage={} reason={} contact={} raw={} source={} source_serial={} raw_ms={} frame_ms={} step={} state={} owner_generation={}.",
            GetTickCount64(),stage,reason,probe.contact,probe.raw,static_cast<unsigned>(source.source),
            source.source_serial,source.sampled_at,frame.sampled_at,frame.physics_step,frame.state,frame.owner_generation);
    } catch(...) {}
}
WriterProbe writer_probe() noexcept {
    if(!pending_writer_input.load(std::memory_order_acquire))return {};
    std::lock_guard lock(mutex);
    return {pending.sequence,contact_identity,raw.serial,observed};
}
WriterProbe retained_writer_probe() noexcept {
    if(!retained_contact_hint.load(std::memory_order_acquire))return {};
    std::lock_guard lock(mutex);
    if(!permitted || !takeoff || !contact_active || !raw.valid || !contact_identity || !fresh(raw_frame,GetTickCount64()))return {};
    return {pending.sequence,contact_identity,raw.serial,raw_frame};
}
void retained_writer_event(const WriterProbe& probe,unsigned stage,const char* reason,
        std::uint32_t state,bool state_known,std::uintptr_t core) noexcept {
    if(!probe.contact || !probe.raw || core!=probe.native.owner.core || stage>=64)return;
    const auto now=GetTickCount64();
    const auto pending_now=pending_writer_input.load(std::memory_order_acquire);
    try {
        std::lock_guard lock(diagnostic_mutex);
        if(retained_writer_count>=128)return;
        if(retained_writer_seen.contact!=probe.contact)retained_writer_seen={probe.contact,0};
        const auto bit=std::uint64_t{1}<<stage;
        if(retained_writer_seen.stages&bit)return;
        retained_writer_seen.stages|=bit;
        retained_writer_rows[retained_writer_count++]={probe,now,pending_now,core,stage,reason,state,state_known};
        retained_writer_queued.store(true,std::memory_order_release);
    } catch(...) {}
}
void writer_event(const WriterProbe& probe,unsigned stage,const char* reason,
        std::uint32_t state,bool state_known,std::uintptr_t core) noexcept {
    if(!probe.input || core!=probe.native.owner.core || stage>=64)return;
    try {
        std::lock_guard lock(diagnostic_mutex);
        if(writer_diagnostic_count>=96)return;
        if(writer_seen.contact!=probe.input)writer_seen={probe.input,0};
        const auto bit=std::uint64_t{1}<<stage;
        if(writer_seen.stages&bit)return;
        writer_seen.stages|=bit;++writer_diagnostic_count;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALLPLANT WRITER: ms={} stage={} reason={} input={} contact={} raw={} callback_core={} owner={} owner_generation={} step={} frame_ms={} frame_state={} state_known={} state={}.",
            GetTickCount64(),stage,reason,probe.input,probe.contact,probe.raw,core,probe.native.owner.core,
            probe.native.owner_generation,probe.native.physics_step,probe.native.sampled_at,probe.native.state,state_known,state);
    } catch(...) {}
}
}
