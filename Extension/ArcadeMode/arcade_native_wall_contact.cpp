#include "arcade_native_wall_contact.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/arcade_wall_contact.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"
#include "Engine/Game/Build/20260929/skater_movement.h"
#include "Engine/Game/Build/image_identity.h"
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_wall_contact {
namespace {
namespace c=game::build::v20260929::arcade_wall_contact;
namespace r=game::build::v20260929::rigid_bodies;
namespace p=game::build::v20260929::physics_bodies;
namespace b=game::build::v20260929::skater_body;
namespace rc=game::build::v20260929::remote_collision;
using Handle=body_hit_identity::NativeHandle;
using Process=void(*)(std::uintptr_t,std::uintptr_t);
Process original{};
std::atomic<bool> permitted{},ready{};
std::uintptr_t image{};
std::mutex mutex;
Snapshot latest{};
std::uint64_t serial{};
unsigned reports{};
unsigned air_steps{},step_reports{};
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
void capture(std::uintptr_t summaries,std::uintptr_t input) {
    if(!permitted.load(std::memory_order_acquire))return;
    NoBailSkater owner{};
    if(!no_bail_skater(owner)||owner.base!=image)return;
    const auto holder=pointer(owner.rig+b::contact_holder_offset);
    if(!holder||pointer(holder+b::contact_struct_offset)!=summaries)return;
    {std::lock_guard lock(mutex);latest={};}
    Snapshot next{};next.owner=owner;next.sampled_at=GetTickCount64();next.physics_thread=GetCurrentThreadId();
    if(!read(input+c::input_seconds,next.seconds)||!std::isfinite(next.seconds)||next.seconds<=0||next.seconds>1||
       !read(owner.context+game::build::v20260929::arcade_native_probe::selected_state,next.state)||
       !read(owner.context+game::build::v20260929::skater_movement::context_velocity,next.velocity)||!finite(next.velocity))return;
    const auto vector=pointer(input+c::input_contacts);
    std::array<std::uintptr_t,2> range{},after{};
    if(!vector||!read(vector,range)||range[1]<range[0]||(range[1]-range[0])%c::record_size||
       (range[1]-range[0])/c::record_size>next.contacts.size())return;
    if(range[1]!=range[0] && !range[0])return;
    next.count=static_cast<std::size_t>((range[1]-range[0])/c::record_size);
    for(std::size_t i=0;i<next.count;++i) {
        const auto at=range[0]+i*c::record_size;auto& contact=next.contacts[i];
        std::uintptr_t own{},other{},own_entity{};
        if(!read(at+c::normal,contact.normal)||!read(at+c::point,contact.point)||
           !read(at+c::bone,contact.bone)||!read(at+c::shape,contact.shape)||
           !read(at+c::own_collidable,own)||!read(at+c::other_collidable,other)||!read(at+c::fallback_kind,contact.kind)||
           contact.bone>=b::body_bone_count||!finite(contact.normal)||!finite(contact.point))return;
        if(own==UINTPTR_MAX||other==UINTPTR_MAX)continue;
        if(other) {
            if(!read(other+c::collidable_entity,contact.other_entity))return;
            if(contact.other_entity && !read(contact.other_entity+c::entity_kind,contact.kind))return;
        }
        if(own && (!read(own+c::collidable_entity,own_entity)||
           (other && own_entity==contact.other_entity)))continue;
        const float norm=std::hypot(contact.normal[0],contact.normal[1],contact.normal[2]);
        if(norm>=.99f && norm<=1.01f)(void)qualify(contact);
    }
    NoBailSkater again{};
    if(!read(vector,after)||range!=after||!no_bail_skater(again)||again!=owner||
       pointer(holder+b::contact_struct_offset)!=summaries)return;
    std::lock_guard lock(mutex);
    if(!permitted.load(std::memory_order_acquire))return;
    next.serial=++serial;next.valid=true;latest=next;
    if(next.state!=200&&next.state!=201)return;
    ++air_steps;
    if(step_reports<40 && (air_steps<=8 || air_steps%120==0)) {
        ++step_reports;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALL CONTACT STEP: ms={} serial={} seconds={} state={} raw_contacts={} physics_thread={}.",
            next.sampled_at,next.serial,next.seconds,next.state,next.count,next.physics_thread);
    }
    for(std::size_t i=0;i<next.count && reports<256;++i) {
        const auto& contact=next.contacts[i];
        if(std::abs(contact.normal[1])>.25f)continue;
        ++reports;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE WALL CONTACT: ms={} serial={} seconds={} state={} bone={} point={},{},{} normal={},{},{} velocity={},{},{} kind={} shape_world={} shape={} generation={} body={} body_generation={} identity={} static={} categories_known={} categories={} wall_admission=unverified.",
            next.sampled_at,next.serial,next.seconds,next.state,contact.bone,contact.point[0],contact.point[1],contact.point[2],
            contact.normal[0],contact.normal[1],contact.normal[2],next.velocity[0],next.velocity[1],next.velocity[2],
            contact.kind,contact.shape.world,contact.shape.index,contact.shape.generation,contact.body.index,
            contact.body.generation,contact.identity_valid,contact.static_body,contact.categories_known,contact.categories);
    }
}
void hooked(std::uintptr_t summaries,std::uintptr_t input) {
    try {capture(summaries,input);} catch(...) {}
    original(summaries,input);
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
    for(const auto& contract:b::contracts)if(!fingerprint(contract))return false;
    if(!fingerprint(rc::shape_valid)||!fingerprint(rc::body_valid)||
       !fingerprint(r::shape_in_categories)||!fingerprint(c::motion_fixed_contract))return false;
    void* relay{};auto* target=reinterpret_cast<void*>(base+c::process);
    if(hook_prepare(target,reinterpret_cast<void*>(&hooked),&relay)!=HookOk)return false;
    original=reinterpret_cast<Process>(relay);
    HookBatch batch;
    if(hook_enable(target)!=HookOk)return false;
    ready.store(true,std::memory_order_release);return true;
}
void enable(bool enabled) noexcept {
    permitted.store(enabled,std::memory_order_release);
    if(!enabled){std::lock_guard lock(mutex);latest={};}
}
Snapshot observe(const NoBailSkater& owner,std::uint64_t now) noexcept {
    std::lock_guard lock(mutex);
    if(!permitted.load(std::memory_order_acquire)||!latest.valid||latest.owner!=owner||
       latest.physics_thread!=GetCurrentThreadId()||
       now<latest.sampled_at||now-latest.sampled_at>150)return {};
    return latest;
}
bool current(const Contact& contact,std::uintptr_t expected_world) noexcept {
    if(!contact.identity_valid||contact.shape.world!=expected_world||contact.body.world!=expected_world)return false;
    auto again=contact;
    return qualify(again)&&again.world_store==contact.world_store&&again.body==contact.body&&again.static_body==contact.static_body&&
        again.categories_known==contact.categories_known&&again.categories==contact.categories;
}
}
