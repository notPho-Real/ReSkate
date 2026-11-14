#include "arcade_native_wall_contact.h"
#include "arcade_mode.h"
#include "arcade_wallplant_runtime.h"
#include "Extension/Profile/local_profile_runtime.h"
#include "Extension/Skater/client_source_spawn.h"
#include "Engine/Game/Build/20260929/no_bail.h"
#include "arcade_contact_owner.h"
#include "arcade_static_world_owner.h"
#include "Engine/Game/World/physics_component_owner.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/arcade_wall_contact.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"
#include "Engine/Game/Build/20260929/skater_movement.h"
#include "Engine/Game/Build/20260929/skater_state.h"
#include "Engine/Game/Build/image_identity.h"
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <Windows.h>
#include <algorithm>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace dingosdk::arcade_mode::native_wall_contact {
namespace {
namespace c=game::build::v20260929::arcade_wall_contact;
namespace r=game::build::v20260929::rigid_bodies;
namespace p=game::build::v20260929::physics_bodies;
namespace b=game::build::v20260929::skater_body;
namespace rc=game::build::v20260929::remote_collision;
namespace s=game::build::v20260929::skater_state;
using Handle=body_hit_identity::NativeHandle;
using Process=void(*)(std::uintptr_t,std::uintptr_t);
Process original{};
using BoardProcess=void(*)(std::uintptr_t,float,const void*);
BoardProcess original_board{};
using CollisionPredicate=bool(*)(std::uintptr_t,float,float,bool);
CollisionPredicate original_collision{};
struct CollisionScope {NoBailSkater owner{};std::uintptr_t collector{};std::uint64_t serial{};};
thread_local CollisionScope collision_scope{};
std::atomic<bool> permitted{},ready{};
std::uintptr_t image{};
std::mutex mutex;
Snapshot latest{};
Snapshot latest_board{};
std::uint64_t serial{};
std::array<std::uint64_t,2> source_serials{};
unsigned reports{};
std::array<unsigned,2> air_steps{},step_reports{};
template<class T> bool read(std::uintptr_t at,T& value) noexcept {return memory::peek(at,value);}
std::uintptr_t pointer(std::uintptr_t at) noexcept {std::uintptr_t value{};return read(at,value)?value:0;}
bool finite(Vec v) noexcept {
    for(float x:v)if(!std::isfinite(x)||std::abs(x)>100000)return false;
    return true;
}
bool qualify(Contact& contact) noexcept {
    if(!ready.load(std::memory_order_acquire)||!contact.shape.world||contact.shape.index>=1048576||contact.shape.generation>255)return false;
    const auto world=contact.shape.world;
    const auto state=pointer(world+p::store_offset);
    if(!state)return false;
    auto reader=[](auto at,auto& out){return read(at,out);};
    const auto world_current=[&]{return pointer(world+p::store_offset)==state;};
    const auto shape_current=[&](const Handle& h){
        std::uint8_t bits{},generation{};
        return world_current() && h.world==world && h.index<1048576 && h.generation<=255 &&
            body_hit_identity::element(reader,pointer(state+r::shape_allocated_bits),h.index>>3,bits) &&
            (bits&(1u<<(h.index&7u))) &&
            body_hit_identity::element(reader,pointer(state+r::shape_generations),h.index,generation) && generation==h.generation;
    };
    const auto allocated=[&](std::uint32_t index){
        std::uint32_t capacity{};std::uint8_t bits{};
        const auto array=pointer(state+r::allocated_bits);
        return read(state+r::body_capacity,capacity) && capacity<=1048576 && index<capacity &&
            body_hit_identity::element(reader,array,index>>3,bits) && (bits&(1u<<(index&7u)));
    };
    const auto body_current=[&](const Handle& h){
        std::uint32_t generation{};
        return h.world==world && allocated(h.index) &&
            body_hit_identity::element(reader,pointer(world+r::world_body_generations),h.index,generation) && generation==h.generation;
    };
    Handle body{};
    if(!body_hit_identity::resolve(world,state,contact.shape,body,reader,world_current,shape_current,allocated,body_current))return false;
    std::uint8_t dynamic{},keyframed{},dynamic_after{},keyframed_after{};
    const auto dynamic_array=pointer(state+r::dynamic_bits);
    const auto keyframed_array=pointer(state+c::keyframed_bits);
    if(!body_hit_identity::element(reader,dynamic_array,body.index>>3,dynamic)||
       !body_hit_identity::element(reader,keyframed_array,body.index>>3,keyframed))return false;
    const auto categories=pointer(state+r::shape_categories);
    std::uint32_t mask{},mask_after{};
    const bool known=body_hit_identity::element(reader,categories,contact.shape.index,mask);
    if(!world_current()||!shape_current(contact.shape)||!body_current(body)||
       pointer(state+r::dynamic_bits)!=dynamic_array||pointer(state+c::keyframed_bits)!=keyframed_array||
       !body_hit_identity::element(reader,dynamic_array,body.index>>3,dynamic_after)||dynamic_after!=dynamic||
       !body_hit_identity::element(reader,keyframed_array,body.index>>3,keyframed_after)||keyframed_after!=keyframed||
       pointer(state+r::shape_categories)!=categories||
       (known && (!body_hit_identity::element(reader,categories,contact.shape.index,mask_after)||mask_after!=mask)))return false;
    contact.body=body;contact.world_store=state;contact.identity_valid=true;
    contact.static_body=((dynamic|keyframed)&(1u<<(body.index&7u)))==0;
    contact.categories=mask;contact.categories_known=known;
    return true;
}
bool capture_raw(Snapshot& next,std::uintptr_t vector,std::size_t body_count) noexcept {
    std::array<std::uintptr_t,2> range{},after{};
    if(!vector||!read(vector,range)||range[1]<range[0]||(range[1]-range[0])%c::record_size||
       (range[1]-range[0])/c::record_size>next.contacts.size())return false;
    if(range[1]!=range[0] && !range[0])return false;
    next.count=static_cast<std::size_t>((range[1]-range[0])/c::record_size);
    for(std::size_t i=0;i<next.count;++i) {
        const auto at=range[0]+i*c::record_size;auto& contact=next.contacts[i];
        std::uintptr_t own{},other{},own_entity{};
        if(!read(at+c::normal,contact.normal)||!read(at+c::point,contact.point)||
           !read(at+c::bone,contact.bone)||!read(at+c::shape,contact.shape)||
           !read(at+c::own_collidable,own)||!read(at+c::other_collidable,other)||!read(at+c::fallback_kind,contact.kind)||
           contact.bone>=body_count||!finite(contact.normal)||!finite(contact.point))return false;
        if(own==UINTPTR_MAX||other==UINTPTR_MAX)continue;
        if(other) {
            if(!read(other+c::collidable_entity,contact.other_entity))return false;
            if(contact.other_entity && !read(contact.other_entity+c::entity_kind,contact.kind))return false;
        }
        if(own && (!read(own+c::collidable_entity,own_entity)||
           (other && own_entity==contact.other_entity)))continue;
        const float norm=std::hypot(contact.normal[0],contact.normal[1],contact.normal[2]);
        if(norm>=.99f && norm<=1.01f)(void)qualify(contact);
    }
    NoBailSkater again{};
    if(!read(vector,after)||range!=after||!no_bail_skater(again)||again!=next.owner)return false;
    next.valid=true;return true;
}
bool begin_snapshot(Snapshot& next,const NoBailSkater& owner,float seconds,Source source) noexcept {
    next.owner=owner;next.seconds=seconds;next.source=source;
    next.sampled_at=GetTickCount64();next.physics_thread=GetCurrentThreadId();
    return std::isfinite(seconds)&&seconds>0&&seconds<=1&&
        read(owner.context+game::build::v20260929::arcade_native_probe::selected_state,next.state)&&
        read(owner.context+game::build::v20260929::skater_movement::context_velocity,next.velocity)&&finite(next.velocity);
}
void publish(Snapshot next) {
    std::lock_guard lock(mutex);
    if(!permitted.load(std::memory_order_acquire))return;
    next.serial=++serial;
    const auto source=static_cast<std::size_t>(next.source);
    next.source_serial=++source_serials[source];
    if(next.source==Source::board)latest_board=next;else latest=next;
    const bool airborne=next.state==200||next.state==201;
    if(!airborne&&next.source!=Source::board)return;
    if(airborne)++air_steps[source];
    if(airborne && step_reports[source]<40 && (air_steps[source]<=8 || air_steps[source]%120==0)) {
        ++step_reports[source];
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALL CONTACT STEP: ms={} serial={} seconds={} state={} raw_contacts={} physics_thread={} source={} source_serial={} state_after={} after_valid={} velocity_after={},{},{}.",
            next.sampled_at,next.serial,next.seconds,next.state,next.count,next.physics_thread,
            next.source==Source::board?"board":"rig",next.source_serial,next.state_after,next.after_valid,
            next.velocity_after[0],next.velocity_after[1],next.velocity_after[2]);
    }
    for(std::size_t i=0;i<next.count && reports<256;++i) {
        const auto& contact=next.contacts[i];
        if(std::abs(contact.normal[1])>.25f)continue;
        ++reports;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALL CONTACT: ms={} serial={} seconds={} state={} bone={} point={},{},{} normal={},{},{} velocity={},{},{} kind={} shape_world={} shape={} generation={} body={} body_generation={} identity={} static={} categories_known={} categories={} source={} source_serial={} state_after={} after_valid={} velocity_after={},{},{} wall_admission=unverified.",
            next.sampled_at,next.serial,next.seconds,next.state,contact.bone,contact.point[0],contact.point[1],contact.point[2],
            contact.normal[0],contact.normal[1],contact.normal[2],next.velocity[0],next.velocity[1],next.velocity[2],
            contact.kind,contact.shape.world,contact.shape.index,contact.shape.generation,contact.body.index,
            contact.body.generation,contact.identity_valid,contact.static_body,contact.categories_known,contact.categories,
            next.source==Source::board?"board":"rig",next.source_serial,next.state_after,next.after_valid,
            next.velocity_after[0],next.velocity_after[1],next.velocity_after[2]);
    }
}
void capture(std::uintptr_t summaries,std::uintptr_t input) {
    if(!permitted.load(std::memory_order_acquire))return;
    NoBailSkater owner{};
    if(!no_bail_skater(owner)||owner.base!=image)return;
    const auto holder=pointer(owner.rig+b::contact_holder_offset);
    if(!holder||pointer(holder+b::contact_struct_offset)!=summaries)return;
    {std::lock_guard lock(mutex);latest={};}
    Snapshot next{};float seconds{};
    if(!read(input+c::input_seconds,seconds)||!begin_snapshot(next,owner,seconds,Source::rig)||
       !capture_raw(next,pointer(input+c::input_contacts),b::body_bone_count)||
       pointer(holder+b::contact_struct_offset)!=summaries)return;
    publish(next);
}
void hooked(std::uintptr_t summaries,std::uintptr_t input) {
    try {capture(summaries,input);} catch(...) {}
    original(summaries,input);
}
std::uintptr_t owned_board(const NoBailSkater& owner) noexcept {
    const auto holder=pointer(owner.core+s::board_holder_offset);
    const auto board=holder?pointer(holder+s::board_physics_offset):0;
    if(!board||pointer(board)!=image+s::board_physics_vtable)return 0;
    const auto bodies=pointer(board+s::physics_bodies_offset);
    std::uint32_t count{};
    return bodies&&read(bodies,count)&&count==s::board_body_count&&
        pointer(bodies+s::body_owner_offset)==board ? board : 0;
}
std::uintptr_t board_vector(std::uintptr_t board) noexcept {
    const auto provider=pointer(board+c::board_provider);
    if(!provider||pointer(provider)!=image+c::provider_vtable||
       pointer(image+c::provider_vtable+0x68)!=image+c::provider_getter)return 0;
    const auto vector=contact_owner::vector(provider,board,[](auto at,auto& out){return read(at,out);});
    return pointer(board+c::board_provider)==provider ? vector : 0;
}
bool capture_board(Snapshot& next,std::uintptr_t board,float seconds) {
    if(!permitted.load(std::memory_order_acquire))return false;
    // A failed owner read for this invocation must not leave a prior board
    // collision available to its later classification callback.
    {std::lock_guard lock(mutex);latest_board={};}
    NoBailSkater owner{};
    if(!no_bail_skater(owner)||owner.base!=image||owned_board(owner)!=board)return false;
    if(!begin_snapshot(next,owner,seconds,Source::board))return false;
    const auto vector=board_vector(board);
    return capture_raw(next,vector,s::board_body_count)&&board_vector(board)==vector;
}
void hooked_board(std::uintptr_t board,float seconds,const void* up) {
    Snapshot next{};bool captured=false;
    try {captured=capture_board(next,board,seconds);} catch(...) {}
    // No altered inputs, native state, extra native call, or repeated response.
    original_board(board,seconds,up);
    if(!captured)return;
    try {
        NoBailSkater after{};
        if(!no_bail_skater(after)||after!=next.owner||owned_board(after)!=board)return;
        next.after_valid=read(after.context+game::build::v20260929::arcade_native_probe::selected_state,next.state_after)&&
            read(after.context+game::build::v20260929::skater_movement::context_velocity,next.velocity_after)&&finite(next.velocity_after);
        if(next.after_valid)publish(next);
    } catch(...) {}
}
bool collision_uncontested(const CollisionScope& scope) noexcept {
    namespace bail=game::build::v20260929::no_bail;
    std::array<std::uint8_t,c::collector_flag_count> flags{};
    std::uint32_t count{},impact{},animation{};
    return scope.serial && scope.collector && pointer(scope.owner.core+c::core_causes)==scope.collector &&
        pointer(scope.collector+c::collector_context)==scope.owner.context &&
        read(scope.collector+c::collector_flags,flags) &&
        std::all_of(flags.begin(),flags.end(),[](auto flag){return flag==0;}) &&
        read(scope.collector+c::collector_count,count) && count==0 &&
        read(scope.owner.context+bail::body_contact_context_offset,impact) &&
        !(impact&(bail::body_contact_context_mask|bail::impact_request_mask)) &&
        read(scope.owner.context+bail::animation_request_offset,animation) && !(animation&bail::animation_request_mask);
}
bool hooked_collision(std::uintptr_t collector,float first,float second,bool mode) {
#if defined(_MSC_VER)
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
#else
    const auto caller=reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
#endif
    const bool result=original_collision(collector,first,second,mode);
    if(!result || caller!=image+c::collision_return || !ready.load(std::memory_order_acquire) ||
       !permitted.load(std::memory_order_acquire) || collision_scope.serial)return result;
    NoBailSkater owner{};
    if(!no_bail_skater(owner))return result;
    Snapshot source{};
    {std::lock_guard lock(mutex);source=latest_board;}
    const auto now=GetTickCount64();
    if(!source.valid || !source.after_valid || source.owner!=owner ||
       source.physics_thread!=GetCurrentThreadId() || now<source.sampled_at || now-source.sampled_at>150 ||
       (source.state!=200 && source.state!=201) || source.state_after!=source.state)return result;
    const CollisionScope scope{owner,collector,source.serial};
    if(!collision_uncontested(scope))return result;
    collision_scope=scope;
    struct Reset {~Reset(){collision_scope={};}} reset;
    bool entered=false;
    try {
        const NativeInputOwner input_owner{owner.base,owner.entity,owner.core,owner.context,owner.rig};
        std::uint64_t at{};
        const auto execution=native_wallplant_context(input_owner,at);
        if(execution.native.valid && native_wallplant_execution_authorized(execution.native,at) &&
           wallplant_runtime::retain_collision_before_response(execution.native,at)) {
            query_arcade_wallplant_collision_geometry();
            if(claim_native_wallplant_contact(execution.native))
                entered=apply_arcade_wallplant_at_collision(execution,source.serial);
        }
    } catch(...) {}
    try {
        static unsigned reported{};
        if(reported<64){
            ++reported;
            logging::log(logging::Level::info,logging::Channel::skater,
                "ARCADE WALLPLANT COLLISION: ms={} raw={} owner={} thread={} original_result={} hold_entered={} state={} state_after={} threshold1={} threshold2={} mode={}.",
                GetTickCount64(),source.serial,owner.core,source.physics_thread,result,entered,
                source.state,source.state_after,first,second,mode);
        }
    } catch(...) {}
    // The original predicate and contact processors ran. Only this
    // producer yields to a committed, qualified plant; no flags/causes are
    // erased and every later native bail test remains in force.
    return entered?false:result;
}
bool fingerprint(const game::build::Fingerprint& contract) noexcept {
    std::array<unsigned char,32> actual{};
    return read(image+contract.rva,actual)&&actual==contract.bytes;
}
}
bool start(std::uintptr_t base) noexcept {
    if(ready.load(std::memory_order_acquire))return base==image;
    if(!base||(image&&image!=base)||!supported_build::running_image_matches(base))return false;
    image=base;
    for(const auto& contract:c::contracts)if(!fingerprint(contract))return false;
    for(const auto& contract:c::board_contracts)if(!fingerprint(contract))return false;
    for(const auto& contract:c::collision_contracts)if(!fingerprint(contract))return false;
    for(const auto& contract:game::build::v20260929::physics_owner::contracts)if(!fingerprint(contract))return false;
    for(const auto& contract:game::build::v20260929::physics_owner::component_contracts)if(!fingerprint(contract))return false;
    for(const auto& contract:b::contracts)if(!fingerprint(contract))return false;
    if(!fingerprint(rc::shape_valid)||!fingerprint(rc::body_valid)||
       !fingerprint(r::shape_in_categories)||!fingerprint(c::motion_fixed_contract))return false;
    void* relay{};auto* target=reinterpret_cast<void*>(base+c::process);
    if(hook_prepare(target,reinterpret_cast<void*>(&hooked),&relay)!=HookOk)return false;
    original=reinterpret_cast<Process>(relay);
    auto* board_target=reinterpret_cast<void*>(base+c::board_process);
    if(hook_prepare(board_target,reinterpret_cast<void*>(&hooked_board),&relay)!=HookOk)return false;
    original_board=reinterpret_cast<BoardProcess>(relay);
    auto* collision_target=reinterpret_cast<void*>(base+c::collision_predicate);
    if(hook_prepare(collision_target,reinterpret_cast<void*>(&hooked_collision),&relay)!=HookOk)return false;
    original_collision=reinterpret_cast<CollisionPredicate>(relay);
    HookBatch batch;
    if(hook_enable(target)!=HookOk)return false;
    if(hook_enable(board_target)!=HookOk)return false;
    if(hook_enable(collision_target)!=HookOk)return false;
    ready.store(true,std::memory_order_release);return true;
}
void enable(bool enabled) noexcept {
    permitted.store(enabled,std::memory_order_release);
    if(!enabled){std::lock_guard lock(mutex);latest={};latest_board={};}
}
bool collision_current(std::uint64_t expected_serial,const NoBailSkater& owner) noexcept {
    if(!expected_serial || collision_scope.serial!=expected_serial || collision_scope.owner!=owner ||
       !permitted.load(std::memory_order_acquire) || !collision_uncontested(collision_scope))return false;
    const auto source=observe(owner,GetTickCount64());
    return source.valid && source.serial==expected_serial && source.source==Source::board && source.after_valid &&
        source.state_after==source.state && (source.state==200 || source.state==201);
}
Snapshot observe(const NoBailSkater& owner,std::uint64_t now) noexcept {
    std::lock_guard lock(mutex);
    if(!permitted.load(std::memory_order_acquire))return {};
    const auto usable=[&](const Snapshot& value) {
        return value.valid&&value.owner==owner&&value.physics_thread==GetCurrentThreadId()&&
            now>=value.sampled_at&&now-value.sampled_at<=150;
    };
    // Preserve the original board path when the later rig path has only empty
    // or self contacts. Native AIR and same-step checks remain in the runtime.
    if(usable(latest_board)&&(latest_board.state==200||latest_board.state==201)) {
        for(std::size_t i=0;i<latest_board.count;++i) {
            const auto& hit=latest_board.contacts[i];
            if(hit.identity_valid&&hit.static_body&&hit.categories_known&&
               hit.kind!=4&&hit.kind!=5&&hit.kind!=8&&hit.kind!=11&&
               std::isfinite(hit.normal[1])&&std::abs(hit.normal[1])<=.1f)return latest_board;
        }
    }
    if(usable(latest))return latest;
    return usable(latest_board)?latest_board:Snapshot{};
}
bool current(const Contact& contact,std::uintptr_t expected_world) noexcept {
    if(!contact.identity_valid||contact.shape.world!=expected_world||contact.body.world!=expected_world)return false;
    auto again=contact;
    return qualify(again)&&again.world_store==contact.world_store&&again.body==contact.body&&again.static_body==contact.static_body&&
        again.categories_known==contact.categories_known&&again.categories==contact.categories;
}
bool static_world_owner(const SurfaceGeometryBody& shape,SurfaceGeometryWorldOwner& out) noexcept {
    out={};
    try {
        Contact first{};first.shape={shape.world,shape.index,shape.generation};
        if(!qualify(first) || !first.static_body || !first.categories_known)return false;
        const auto current_owner=[&] {
            auto again=first;
            return qualify(again) && again.static_body && again.categories_known &&
                again.world_store==first.world_store && again.body==first.body && again.categories==first.categories;
        };
        return static_world_owner_detail::observe(shape,
            SurfaceGeometryBody{first.body.world,first.body.index,first.body.generation},first.world_store,
            [](auto at,auto& value){return read(at,value);},current_owner,out);
    } catch(...) {out={};return false;}
}
bool local_component_owner(const SurfaceGeometryBody& shape,std::uintptr_t entity,SurfaceGeometryWorldOwner& out) noexcept {
    out={};
    try {
        NoBailSkater local{};
        if(!no_bail_skater(local) || local.base!=image || local.entity!=entity)return false;
        Contact first{};first.shape={shape.world,shape.index,shape.generation};
        if(!qualify(first))return false;
        const auto current_owner=[&] {
            NoBailSkater again{};auto contact=first;
            return no_bail_skater(again) && again==local && qualify(contact) &&
                contact.world_store==first.world_store && contact.body==first.body &&
                contact.categories_known==first.categories_known && contact.categories==first.categories;
        };
        const auto reader=[](auto at,auto& value){return read(at,value);};
        const auto observe=[&](SurfaceGeometryWorldOwner& stamp) {
            return static_world_owner_detail::observe(shape,
                SurfaceGeometryBody{first.body.world,first.body.index,first.body.generation},first.world_store,
                reader,current_owner,stamp) &&
                (physics_component_owner::belongs_to(image,entity,stamp.value,
                    Handle{shape.world,stamp.index,stamp.generation},reader,current_owner) ||
                 physics_component_owner::belongs_to_board(image,entity,stamp.value,
                    Handle{shape.world,stamp.index,stamp.generation},reader,current_owner));
        };
        SurfaceGeometryWorldOwner before{},after{};
        if(!observe(before) || !observe(after) || before!=after || !current_owner())return false;
        out=before;return true;
    }catch(...){out={};return false;}
}
}
