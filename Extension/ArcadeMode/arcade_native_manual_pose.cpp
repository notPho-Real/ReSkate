#include "arcade_native_manual_pose.h"
#include "Extension/Skater/no_bail.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/arcade_manual_pose.h"
#include "Engine/Game/Skater/native_orientation.h"
#include <atomic>
#include <cmath>
#include <mutex>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_manual_pose {
namespace {
namespace contract=game::build::v20260929::arcade_manual_pose;
std::atomic<bool> permitted{};
std::mutex mutex;
std::uintptr_t image{},block{};
NoBailSkater owner{};
std::uint64_t serial{},sampled_at{};
struct Actor {std::uintptr_t object{};std::uint64_t reported_at{};bool initialized{};};
std::array<Actor,contract::actors.size()> actors{};
unsigned initialization_reports{};
template<class T> bool read(std::uintptr_t at,T& value) noexcept {return memory::peek(at,value);}
std::uintptr_t pointer(std::uintptr_t at) noexcept {std::uintptr_t p{};return read(at,p)?p:0;}
std::uintptr_t block_for(const NoBailSkater& skater) noexcept {
    const auto collection=pointer(skater.entity+contract::collection);std::uint8_t count{};
    if (!collection || pointer(collection+contract::collection_owner)!=skater.entity ||
        !read(collection+contract::collection_count,count) || count>128) return 0;
    for(unsigned i=0;i<count;++i) {
        const auto component=pointer(collection+contract::components+i*contract::component_stride);
        if(component && pointer(component)==image+contract::component_type) {
            const auto result=pointer(pointer(pointer(component+contract::animatable)+contract::holder)+contract::state_block);
            return pointer(result)==image+contract::block_type?result:0;
        }
    }
    return 0;
}
bool current() noexcept {
    const auto now=GetTickCount64();NoBailSkater actual{};std::uint32_t state{};
    return permitted.load() && block && now>=sampled_at && now-sampled_at<=150 &&
        no_bail_skater(actual) && actual==owner && block_for(actual)==block &&
        read(owner.context+contract::physics_state,state) && (state==100 || state==101);
}
int actor_index(std::uintptr_t actor) noexcept {
    const auto asset=pointer(actor+contract::actor_asset);contract::Guid guid{};
    if(asset<0x10010 || pointer(asset)!=image+contract::actor_type || !read(asset-16,guid))return -1;
    for(unsigned i=0;i<contract::actors.size();++i)if(guid==contract::actors[i].guid)return static_cast<int>(i);
    return -1;
}
}
void tick(std::uintptr_t base,bool gameplay) noexcept {
    permitted.store(gameplay);
    std::lock_guard lock(mutex);
    if(!gameplay){block=0;actors={};return;}
    if(!base || (image && image!=base))return;
    image=base;NoBailSkater actual{};
    const auto next=no_bail_skater(actual)?block_for(actual):0;
    if(!next || actual!=owner || next!=block){owner=actual;block=next;actors={};++serial;}
    sampled_at=GetTickCount64();
}
void initialized(std::uintptr_t actor,std::uintptr_t view) noexcept {
    if(!permitted.load())return;
    std::lock_guard lock(mutex);float duration{};
    const auto index=actor_index(actor);
    if(index<0)return;
    const bool owner_current=current(),view_matches=view==block+contract::state_view;
    const bool duration_valid=read(actor+contract::actor_duration,duration) &&
        std::isfinite(duration) && duration>0 && duration<=10000;
    if(initialization_reports<32) {
        ++initialization_reports;
        logging::log(logging::Level::info,logging::Channel::skater,
            "ARCADE NATIVE MANUAL INIT: pose={} owner_current={} view_matches={} duration_valid={} admitted={}.",
            contract::actors[static_cast<unsigned>(index)].name,owner_current,view_matches,duration_valid,
            owner_current && view_matches && duration_valid);
    }
    if(!owner_current || !view_matches || !duration_valid)return;
    actors[static_cast<unsigned>(index)]={actor,0,true};
}
Evaluation before_evaluate(std::uintptr_t actor,std::uintptr_t context,float delta) noexcept {
    if(!permitted.load() || !std::isfinite(delta) || delta<=0)return {};
    std::lock_guard lock(mutex);float before{};
    const auto index=actor_index(actor);
    if(index<0 || !current() || pointer(context+contract::evaluation_view)!=block+contract::state_view)return {};
    const auto i=static_cast<unsigned>(index);
    if(!actors[i].initialized || actors[i].object!=actor ||
        !read(actor+contract::actor_time,before) || !std::isfinite(before))return {};
    return {actor,before,serial,i};
}
void after_evaluate(Evaluation token,std::uintptr_t context,float delta) noexcept {
    if(!token.actor)return;
    std::lock_guard lock(mutex);float after{};std::uint32_t state{},contacts{};
    if(!current() || token.serial!=serial || token.index>=actors.size() ||
        actors[token.index].object!=token.actor || actor_index(token.actor)!=static_cast<int>(token.index) ||
        pointer(context+contract::evaluation_view)!=block+contract::state_view ||
        !read(token.actor+contract::actor_time,after) || !std::isfinite(after) || after<=token.before ||
        !read(owner.context+contract::physics_state,state) || !read(owner.context+contract::contact_word,contacts))return;
    const auto now=GetTickCount64();auto& actor=actors[token.index];
    if(actor.reported_at && now>=actor.reported_at && now-actor.reported_at<250)return;
    const auto orientation=read_native_orientation([](auto at,auto& value){return memory::peek(at,value);},owner.core,owner.context);
    actor.reported_at=now;
    logging::log(logging::Level::info,logging::Channel::skater,
        "ARCADE NATIVE MANUAL POSE: ms={} serial={} pose={} initialized=true before={} after={} delta={} state={} contacts={} orientation_valid={} fakie={} blend_weight=unverified scoring_acceptance=unverified.",
        now,serial,contract::actors[token.index].name,token.before,after,delta,state,contacts&0xc000u,orientation.valid,orientation.fakie);
}
}
