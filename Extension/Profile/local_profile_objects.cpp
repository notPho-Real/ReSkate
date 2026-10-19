#include "Extension/ArcadeMode/arcade_mode.h"
#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include "Extension/ArcadeMode/arcade_surface_geometry_native.h"
#include "Extension/Objects/ParkEditor/park_editor_internal.h"
#include "Extension/Customization/local_customization_runtime.h"
#include "Extension/Skater/no_bail.h"
#include "Engine/Game/Build/20260929/puppet_cost.h"
#include "runtime_internal.h"
#include "Extension/Objects/local_placements_runtime.h"
#include "Extension/Objects/teleport_transform.h"
#include "Extension/Objects/ParkEditor/park_editor_runtime.h"
#include "Extension/Objects/ParkEditor/park_editor_surface.h"
#include <cmath>
#include "Extension/World/local_world_layers.h"

namespace dingosdk {
using namespace profile_runtime;
ObjectPersistenceModel local_profile_object_persistence() {
    std::lock_guard lock(local_runtime().native_mutex);
    auto& r = placements_runtime();
    ObjectPersistenceModel model;
    model.available = local_runtime().active.load() && r.store && !r.failed && !r.save_failed;
    model.enabled = r.enabled; model.map = r.map; model.clearing = r.clearing;
    if (const auto it = r.document.maps.find(r.map); it != r.document.maps.end()) model.count = it->second.size();
    model.busy = r.clearing || r.inflight.has_value();
    for (const auto& row : r.rows) model.rows.push_back({row.token, row.object.item, row.object.position, row.saved_id != 0, row.spawned});
    model.can_clear = model.available && world_layers_runtime().model.ready && !r.map.empty() &&
        !model.busy && model.count != 0;
    model.status = r.status;
    if (r.save_failed) model.status = "Object save failed. The previous file is retained; restart after checking objects.sqlite3.";
    if (!r.store || r.failed) model.status = "Object persistence is unavailable. Check objects.sqlite3 and the runtime log.";
    return model;
}
bool set_local_object_persistence(bool enabled) {
    std::lock_guard lock(local_runtime().native_mutex);
    auto& r = placements_runtime();
    if (!local_runtime().active || !r.store || r.failed || r.save_failed || park_editor_owns_placements()) return false;
    try {
        local_runtime().store->set_bool_option("ReSkate.ObjectPersistence", enabled);
        r.enabled = enabled;
        r.status = enabled ? "Object persistence enabled." : "Object persistence disabled. Saved layouts are retained.";
        return true;
    } catch (...) { r.status = "Could not save the object persistence option."; return false; }
}
bool clear_local_persisted_objects(std::string_view map) {
    std::lock_guard lock(local_runtime().native_mutex);
    auto& r = placements_runtime();
    if (!local_runtime().active || !placement_session_ready() || r.map != map) return false;
    std::set<std::uint64_t> tokens;
    for (const auto& row : r.rows) if (row.saved_id) tokens.insert(row.token);
    return begin_placement_delete(tokens);
}
bool delete_local_placed_object(std::string_view map, std::uint64_t token) {
    std::lock_guard lock(local_runtime().native_mutex);
    auto& r = placements_runtime();
    if (!local_runtime().active || !placement_session_ready() || r.map != map ||
        std::none_of(r.rows.begin(), r.rows.end(), [&](const auto& row) { return row.token == token; })) return false;
    return begin_placement_delete({token});
}
bool teleport_local_skater(const std::array<float, 3>& position) {
    return teleport_local_skater(position, 0);
}
bool teleport_local_skater(const std::array<float, 3>& position, float heading) {
    const auto transform = upright_teleport_transform(position, heading);
    if (!transform) return false;
    std::lock_guard lock(local_runtime().native_mutex);
    auto& r = placements_runtime();
    if (!local_runtime().active || !r.teleport || !r.transition_ctor || !r.transition_destroy) return false;
    r.position_teleport = *transform;
    r.position_teleport_at = GetTickCount64();
    return true;
}
std::optional<float> local_ground_height(float x, float z, float top, float bottom) {
    std::lock_guard lock(local_runtime().native_mutex);
    auto& r = placements_runtime();
    if (!local_runtime().active || !r.context) return {};
    for (const auto v : {x, z, top, bottom})
        if (!std::isfinite(v) || std::abs(v) > 1e5f) return {};
    if (!(top > bottom)) return {};
    static std::uintptr_t api_base{};
    static editor::NativeSurfaceApi api;
    if (api_base != local_runtime().base) { api = editor::surface_api(local_runtime().base); api_base = local_runtime().base; }
    if (!api.ready) return {};
    // The client physics world, resolved from the client TLS context like the park editor's
    // surface probe (the server context has no world).
    std::array<std::uintptr_t, 2> context{};
    r.context(context.data());
    if (!context[0]) return {};
    const auto world = api.world(context[0]);
    if (!world) return {};
    // Closest hit from the top: the topmost surface (a roof or deck above the street wins).
    const auto hit = editor::cast_surface(api, world, {x, top, z}, {x, bottom, z});
    if (!hit) return {};
    return (*hit)[1];
}
bool teleport_to_local_placed_object(std::string_view map, std::uint64_t token) {
    std::lock_guard lock(local_runtime().native_mutex);
    auto& r = placements_runtime();
    if (!local_runtime().active || !placement_session_ready() || r.map != map || r.clearing ||
        std::none_of(r.rows.begin(), r.rows.end(), [&](const auto& row) { return row.token == token; })) return false;
    r.teleport_token = token; r.teleport_queued_at = GetTickCount64();
    r.status = "Teleport queued...";
    return true;
}
namespace {
using namespace arcade_mode;
// Exact self-ECS bridge from davad00/ReSkate (GPL-3.0),
// 992e35fddb5bcef914e2ec20b13e9316393b6763, traffic_vehicle_surface.cpp
// self_construct/self_fields/self_attach and skater_identity/query_identity.
// No traffic/car-grab behavior or legacy-pointer-to-ECS conversion is used.
constexpr std::array surface_self_contracts{
 game::build::Fingerprint{0xf88670,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x56,0x57,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0xf0,0x00,0x00,0x00,0x44,0x0f,0xb7,0x05,0xff,0x85,0x2f}},
 game::build::Fingerprint{0x104bee0,{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8d,0x05,0x8f,0x18,0x13,0x05,0x48,0x89,0x51,0x08,0x48,0x8b,0xf9,0x4c,0x89,0x41,0x10,0x49}},
 game::build::Fingerprint{0x105d0c3,{0x4c,0x8b,0x4e,0x20,0x4c,0x8b,0xc3,0x89,0x4c,0x24,0x20,0x48,0x8b,0xc8,0xe8,0xda,0xda,0xfc,0xff,0x48,0x8b,0x08,0x48,0x89,0x4e,0x18,0x4c,0x85,0x76,0x18,0x4c,0x89}}};
struct SurfaceSelf {
 std::uintptr_t collection{},component{};std::uint64_t entity{};
 alignas(16) std::array<std::uint64_t,3> query{};
 bool operator==(const SurfaceSelf&) const=default;
};
bool read_surface_self(std::uintptr_t context,const NoBailSkater& owner,SurfaceSelf& result) {
 auto& r=placements_runtime();SurfaceSelf next;
 std::uintptr_t actual{},type{},component{};std::uint8_t count{},attached{};
 if(!r.query||!r.valid||!memory::read(owner.entity+0x20,actual)||actual!=context||
    !memory::read(owner.entity+0x70,next.collection)||!next.collection||
    !memory::read(next.collection,actual)||actual!=owner.entity||!memory::read(next.collection+8,count)||count>128)return false;
 for(unsigned i=0;i<count;++i){
  std::uintptr_t candidate{},back{};
  if(!memory::read(next.collection+0x20+i*0x20,candidate))return false;
  if(!candidate)continue;
  if(!memory::read(candidate,type))return false;
  if(type!=owner.base+game::build::v20260929::puppet_cost::ecs_vtable)continue;
  if(component||!memory::read(candidate+0x18,back)||back!=next.collection)return false;
  component=candidate;
 }
 if(!component||!memory::read(component+0x40,type)||type!=owner.base+0x61714f8||
    !memory::read(component+0x50,actual)||actual!=context||!memory::read(component+0x60,actual)||actual!=owner.entity||
    !memory::read(component+0x79,attached)||attached!=1||!memory::read(component+0x58,next.entity)||!(next.entity>>32))return false;
 alignas(16) std::array<std::uint64_t,3> query{},after{};
 alignas(16) const std::array<std::uint64_t,2> reference{next.entity,0};
 std::uint32_t space{},incarnation{},after_incarnation{};
 if(!r.valid(reference.data())||r.query(query.data(),next.entity)!=query.data()||query[0]!=next.entity||!query[1]||!query[2]||
    !memory::read(query[1]+8,space)||space!=std::uint32_t(next.entity>>32)||
    !memory::read(query[2],incarnation)||incarnation!=std::uint32_t(next.entity)||
    r.query(after.data(),next.entity)!=after.data()||after!=query||!r.valid(reference.data())||
    !memory::read(after[2],after_incarnation)||after_incarnation!=incarnation)return false;
 next.component=component;next.query=query;result=next;return true;
}
struct SurfaceCallback {
 std::unique_lock<std::recursive_mutex>& native_lock;
 std::unique_lock<std::recursive_mutex>& world_lock;
 DWORD thread{};std::uintptr_t client{};NoBailSkater owner;SurfaceSelf self;
 SurfaceGeometryContext context;editor::NativeSurfaceApi surface;editor::NativePickingApi picking;
 bool entered{};
};
bool surface_callback_current(void* user,const SurfaceGeometryContext& context) noexcept {
 try{
  const auto& c=*static_cast<SurfaceCallback*>(user);auto& local=local_runtime();auto& world=world_layers_runtime();
  const auto& editor=park_editor_detail::editor_state();
  if(!c.entered||!c.native_lock.owns_lock()||!c.world_lock.owns_lock()||c.thread!=GetCurrentThreadId()||
     cosmetic_runtime().update_thread!=c.thread||!local.active||local.base!=context.owner[0]||context!=c.context||
     world.root_generation!=context.world_generation||!world.model.ready||
     std::none_of(world.nodes.begin(),world.nodes.end(),[&](const auto& node){return node.second.generation==context.world_generation&&node.second.context==world.context;})||
     !editor.surface_api.ready||!editor.picking_api.ready||editor.surface_api.world!=c.surface.world||editor.surface_api.ray!=c.surface.ray||
     editor.surface_api.scope_allocator!=c.surface.scope_allocator||editor.surface_api.release!=c.surface.release||
     editor.picking_api.hit_body!=c.picking.hit_body||editor.picking_api.body_valid!=c.picking.body_valid||editor.picking_api.body_entity!=c.picking.body_entity)return false;
  std::array<std::uintptr_t,2> client_context{};auto& placements=placements_runtime();
  if(!placements.context)return false;
  placements.context(client_context.data());if(client_context[0]!=context.client_context)return false;
  std::uintptr_t actual{};NoBailSkater owner{};SurfaceSelf self;
  if(!memory::read(c.client+8,actual)||actual!=context.client_context||!no_bail_skater(owner)||owner!=c.owner||
     !read_surface_self(context.client_context,owner,self)||self!=c.self||self.entity!=context.self_entity)return false;
  const auto frame=surface_geometry_native_frame(GetTickCount64());const auto current=InputNativeContext::from_frame(frame);
  return frame.valid&&frame.owner==NativeInputOwner{context.owner[0],context.owner[1],context.owner[2],context.owner[3],context.owner[4]}&&
   context.self_generation==frame.owner_generation&&current.fresh(GetTickCount64())&&context.native.fresh(GetTickCount64())&&
   current.generations==context.native.generations&&current.air_episode()==context.native.air_episode()&&current.airborne==context.native.airborne&&
   current.physics_step>=context.native.physics_step&&current.sampled_at>=context.native.sampled_at;
 }catch(...){return false;}
}
}
void tick_arcade_surface_geometry(std::uintptr_t client) noexcept {
 PreserveError preserve;arcade_mode::invalidate_surface_geometry();
 if(!arcade_mode::enabled())return;
 try{
  auto& local=local_runtime();std::unique_lock native(local.native_mutex);
  auto& world=world_layers_runtime();std::unique_lock lifetime(world.lifetime_mutex);
  if(!local.active||cosmetic_runtime().update_thread!=GetCurrentThreadId()||!world.model.ready||!world.root_generation)return;
  const auto frame=arcade_mode::surface_geometry_native_frame(GetTickCount64());NoBailSkater owner{};
  if(!frame.valid||!no_bail_skater(owner)||frame.owner!=NativeInputOwner{owner.base,owner.entity,owner.core,owner.context,owner.rig}||owner.base!=local.base)return;
  static std::uintptr_t validated_base{};static bool self_ready{};
  if(validated_base!=local.base){
   validated_base=local.base;self_ready=std::all_of(surface_self_contracts.begin(),surface_self_contracts.end(),[&](const auto& contract){
    std::array<unsigned char,32> bytes{};return memory::read_bytes(local.base+contract.rva,bytes.data(),bytes.size())&&bytes==contract.bytes;});
  }
  auto& placements=placements_runtime();const auto& editor=park_editor_detail::editor_state();
  std::array<std::uintptr_t,2> context{};std::uintptr_t actual{};
  if(!self_ready||!placements.context||!editor.surface_api.ready||!editor.picking_api.ready)return;
  placements.context(context.data());
  if(!context[0]||!memory::read(client+8,actual)||actual!=context[0])return;
  SurfaceSelf self;if(!read_surface_self(context[0],owner,self))return;
  const auto physics_world=editor.surface_api.world(context[0]);if(!physics_world)return;
  SurfaceGeometryContext origin{{owner.base,owner.entity,owner.core,owner.context,owner.rig},InputNativeContext::from_frame(frame),
   context[0],physics_world,world.root_generation,self.entity,frame.owner_generation,SurfaceGeometryPhase::client};
  SurfaceCallback callback{native,lifetime,GetCurrentThreadId(),client,owner,self,origin,editor.surface_api,editor.picking_api,true};
  SurfaceGeometryNativeApi api;api.base=local.base;api.surface=callback.surface;
  api.picking={callback.picking.hit_body,callback.picking.body_valid,callback.picking.body_entity,callback.picking.ready};
  api.user=&callback;api.now=[](void*)noexcept->std::uint64_t{return GetTickCount64();};api.current=&surface_callback_current;api.read_bytes=&memory::read_bytes;
  alignas(16) std::array<float,12> pose{};
  if(!placements.pose||placements.pose(pose.data(),self.query.data())!=pose.data()||!surface_callback_current(&callback,origin))return;
  SurfaceGeometryRequest request;
  constexpr std::array<std::array<float,2>,5> offsets{{{0,0},{-.25f,0},{.25f,0},{0,-.25f},{0,.25f}}};
  for(std::size_t i=0;i<5;++i)request.segments[i]={{pose[8]+offsets[i][0],pose[9]+.75f,pose[10]+offsets[i][1]},
   {pose[8]+offsets[i][0],pose[9]-2.f,pose[10]+offsets[i][1]}};
  auto sample=query_surface_geometry(api,origin,request);
  if(sample.valid&&!revalidate_surface_geometry(api,origin,sample))sample={};
  publish_surface_geometry(sample);callback.entered=false;
 }catch(...){arcade_mode::invalidate_surface_geometry();}
}

}
