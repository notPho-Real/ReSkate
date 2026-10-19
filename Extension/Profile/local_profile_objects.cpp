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
#include "Extension/World/local_population_controls.h"
#include "Extension/World/local_park_rotation.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/20260929/park_rotation.h"
#include "Engine/Game/Build/20260929/population.h"
#include "Engine/Game/Build/20260929/remote_collision.h"
#include "Engine/Game/Build/20260929/world_layers.h"
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
 game::build::Fingerprint{0x104bee0,{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8d,0x05,0x8f,0x18,0x13,0x05,0x48,0x89,0x51,0x08,0x48,0x89,0x01,0x48,0x8b,0xf9,0x4c,0x89,0x41,0x10,0x49}},
 game::build::Fingerprint{0x105d0c3,{0x4c,0x8b,0x4e,0x20,0x4c,0x8b,0xc3,0x89,0x4c,0x24,0x20,0x48,0x8b,0xc8,0xe8,0xda,0xda,0xfc,0xff,0x48,0x8b,0x08,0x48,0x89,0x4e,0x18,0x4c,0x85,0x76,0x18,0x4c,0x89}}};
struct SurfaceClientGate {std::uintptr_t client{},base{},context{};std::uint64_t sampled_at{};};
SurfaceClientGate surface_client_gate; // Existing client callback, protected by native_mutex.
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
  surface_client_gate={client,local.base,context[0],GetTickCount64()};
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

// Scope/table/selected-pose reuse: davad00/ReSkate (GPL-3.0),
// 992e35fddb5bcef914e2ec20b13e9316393b6763, traffic_vehicle_provider.cpp.
// Selected traffic observation runtime. All native queries execute only inside
// the existing post-update park callback and existing native/world locks.
namespace {
struct TrafficScope {
 std::uintptr_t client{},client_context{},server{},server_context{},park{},park_context{},root{},root_data{},root_owner{},traffic{};
 std::uint64_t world_generation{};
 bool operator==(const TrafficScope&)const=default;
};
struct TrafficRuntime {
 TrafficSurfaceProvider provider;
 bool observing{};
 TrafficScope scope;
 TrafficSurfaceSelection selected;
 std::array<std::uint64_t,3> query{};
 SurfaceGeometryBody body;
};
TrafficRuntime& traffic_runtime(){static TrafficRuntime value;return value;} // native_mutex only
bool traffic_pointer(std::uintptr_t p){return p>=0x10000&&p<=0x00007fffffffffefULL;}
bool traffic_scope(std::uintptr_t client,std::uintptr_t park,TrafficScope& out){
 const auto base=local_runtime().base;auto& w=world_layers_runtime();auto& controls=world_control_runtime();
 auto& parks=park_runtime();
 if(!base||!traffic_pointer(client)||!traffic_pointer(park)||!local_runtime().active||!parks.active.load(std::memory_order_acquire)||!w.active.load(std::memory_order_acquire)||
    !controls.population_active.load(std::memory_order_acquire)||parks.surface_thread!=GetCurrentThreadId()||parks.surface_park!=park||!w.model.ready||w.model.map==WorldMap::none||!w.root_generation)return false;
 TrafficScope s;s.client=client;s.park=park;s.world_generation=w.root_generation;
 const auto read=[](auto at,auto& value){return memory::read(at,value);};
 std::uintptr_t value{},controller{},reference{};std::uint32_t state{},online{},offset{};std::uint8_t flag{};int realm{};
 if(!read(client,value)||value!=base+addr::engine::client_vtable||!read(client+8,s.client_context)||!s.client_context||
    !read(client+0xc4,state)||(state!=13&&state!=21)||!read(client+0xc0,online)||online>1||
    !read(base+addr::engine::game_server,s.server)||!s.server||!read(s.server,value)||value!=base+addr::engine::server_vtable||
    !read(client+0xd0,controller)||!controller||!read(controller,value)||value!=base+addr::engine::controller_vtable||
    !read(controller+0x4c0,value)||value!=s.server||!read(s.server+8,s.server_context)||!s.server_context||
    !read(base+addr::remote_collision::game_context_global,value)||value!=s.client_context||
    !read(base+addr::remote_collision::game_context_global+8,value)||value!=s.server_context||
    !read(base+addr::park_rotation::manager,value)||value!=park||!park||!read(park,value)||value!=base+addr::park_rotation::manager_vtable||
    !read(park+8,reference)||reference!=s.server_context||!read(base+addr::park_rotation::context_offset,offset)||offset>0x1000000||
    reference>UINTPTR_MAX-offset||!read(reference+offset+0x28,flag)||flag!=1||w.context!=s.server_context)return false;
 s.park_context=reference+offset;
 if(!read(base+addr::population::managers+8,s.traffic)||!s.traffic||!read(s.traffic,value)||value!=base+addr::population::manager_vtable||
    !read(s.traffic+8,realm)||realm!=1||!read(s.traffic+0x152,flag)||flag!=1||!read(s.traffic+0x150,flag)||flag!=1)return false;
 for(const auto& [entity,node]:w.nodes){
  if(node.generation!=w.root_generation||node.context!=w.context)continue;
  if(s.root)return false;s.root=entity;s.root_data=node.data;
 }
 if(!s.root||!read(s.root,value)||value!=base+addr::world_layers::subworld_reference_vtable||!read(s.root+0x38,value)||value!=s.root_data||
    !read(s.root+0x30,s.root_owner)||!s.root_owner||!read(s.root_owner+0x20,value)||value!=s.server_context||
    !read(s.root+0x80,value)||!value||!read(s.root+0x148,flag)||!(flag&1))return false;
 for(const auto p:{s.client_context,s.server,s.server_context,controller,s.root,s.root_owner,s.traffic})if(!traffic_pointer(p))return false;
 out=s;return true;
}
bool traffic_members(std::uintptr_t manager,std::vector<std::uint64_t>& out){
 std::uintptr_t begin{},end{};if(!memory::read(manager+0xa8,begin)||!memory::read(manager+0xb0,end)||end<begin||
    end-begin>8000||begin%8||end%8||(end!=begin&&!begin))return false;
 std::vector<std::uint64_t> ids;std::set<std::uint64_t> unique;
 for(auto at=begin;at<end;at+=8){std::uint64_t id{};if(!memory::read(at,id)||!(id>>32)||!unique.insert(id).second)return false;ids.push_back(id);}
 std::uintptr_t after_begin{},after_end{};
 if(!memory::read(manager+0xa8,after_begin)||after_begin!=begin||!memory::read(manager+0xb0,after_end)||after_end!=end)return false;
 out=std::move(ids);return true;
}
// POD-only public native boundaries; no mutex/object construction in __try.
// Missing optional parent/peer contracts deliberately remain unavailable.
bool traffic_valid(const void* ref){
#ifdef _MSC_VER
 __try{return placements_runtime().valid&&placements_runtime().valid(ref);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
#else
 try{return placements_runtime().valid&&placements_runtime().valid(ref);}catch(...){return false;}
#endif
}
void* traffic_query(void* out,std::uint64_t id){
#ifdef _MSC_VER
 __try{return placements_runtime().query?placements_runtime().query(out,id):nullptr;}__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
#else
 try{return placements_runtime().query?placements_runtime().query(out,id):nullptr;}catch(...){return nullptr;}
#endif
}
float* traffic_pose(float* out,const void* query){
#ifdef _MSC_VER
 __try{return placements_runtime().pose?placements_runtime().pose(out,query):nullptr;}__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
#else
 try{return placements_runtime().pose?placements_runtime().pose(out,query):nullptr;}catch(...){return nullptr;}
#endif
}
bool traffic_body_valid(const void* body){
#ifdef _MSC_VER
 __try{return park_editor_detail::editor_state().picking_api.body_valid(body);}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
#else
 try{return park_editor_detail::editor_state().picking_api.body_valid(body);}catch(...){return false;}
#endif
}
void* traffic_body_entity(void* out,const void* body){
#ifdef _MSC_VER
 __try{return park_editor_detail::editor_state().picking_api.body_entity(out,body);}__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
#else
 try{return park_editor_detail::editor_state().picking_api.body_entity(out,body);}catch(...){return nullptr;}
#endif
}
std::uintptr_t traffic_mapping(){
#ifdef _MSC_VER
 __try{return park_editor_detail::editor_state().picking_api.mapping();}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
#else
 try{return park_editor_detail::editor_state().picking_api.mapping();}catch(...){return 0;}
#endif
}
void* traffic_map(std::uintptr_t manager,void* out,std::uint64_t id,int direction){
#ifdef _MSC_VER
 __try{return park_editor_detail::editor_state().picking_api.map_entity(manager,out,id,direction);}__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
#else
 try{return park_editor_detail::editor_state().picking_api.map_entity(manager,out,id,direction);}catch(...){return nullptr;}
#endif
}
std::uint64_t traffic_mapped(std::uint64_t id){
 const auto manager=traffic_mapping();std::uintptr_t buckets{};std::uint32_t count{},entries{};std::uint64_t out{};
 if(!manager||!memory::read(manager+0x70,buckets)||!buckets||!memory::read(manager+0x78,count)||!count||count>0x100000||
    !memory::read(manager+0x7c,entries)||!entries||traffic_map(manager,&out,id,1)!=&out)return 0;return out;
}
double traffic_seconds(void*)noexcept{
 LARGE_INTEGER frequency{},counter{};
 return QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0&&QueryPerformanceCounter(&counter)&&counter.QuadPart>0?
  static_cast<double>(counter.QuadPart)/static_cast<double>(frequency.QuadPart):0;
}
struct TrafficCallback {
 SurfaceCallback base;
 TrafficScope scope;
 std::vector<std::uint64_t> members;
 TrafficSurfaceSelection selected;
 std::array<std::uint64_t,3> query;
 decltype(placements_runtime().valid) valid{};
 decltype(placements_runtime().query) query_api{};
 decltype(placements_runtime().pose) pose_api{};
};
bool traffic_callback_current(void* user,const SurfaceGeometryContext& context,TrafficSurfaceSelection selected)noexcept{
 try{
  const auto& c=*static_cast<TrafficCallback*>(user);const auto& b=c.base;auto& e=park_editor_detail::editor_state();
  const auto& placements=placements_runtime();
  if(!c.valid||!c.query_api||!c.pose_api||placements.valid!=c.valid||placements.query!=c.query_api||placements.pose!=c.pose_api)return false;
  if(!b.entered||!b.native_lock.owns_lock()||!b.world_lock.owns_lock()||b.thread!=GetCurrentThreadId()||context!=b.context||
     selected!=c.selected||!population_surface_current(selected)||!arcade_mode::enabled()||local_runtime().base!=context.owner[0]||
     !e.surface_api.ready||!e.picking_api.ready||e.surface_api.world!=b.surface.world||e.surface_api.ray!=b.surface.ray||
     e.surface_api.scope_allocator!=b.surface.scope_allocator||e.surface_api.release!=b.surface.release||
     e.picking_api.body_valid!=b.picking.body_valid||e.picking_api.body_entity!=b.picking.body_entity||e.picking_api.hit_body!=b.picking.hit_body||
     e.picking_api.mapping!=b.picking.mapping||e.picking_api.map_entity!=b.picking.map_entity)return false;
  const auto now=GetTickCount64();const auto gate=surface_client_gate;
  if(gate.client!=b.client||gate.base!=context.owner[0]||gate.context!=context.client_context||now<gate.sampled_at||now-gate.sampled_at>150||
     b.surface.world(context.client_context)!=context.world)return false;
  TrafficScope scope;std::vector<std::uint64_t> ids;NoBailSkater owner{};SurfaceSelf self;
  if(!traffic_scope(b.client,c.scope.park,scope)||scope!=c.scope||!traffic_members(scope.traffic,ids)||ids!=c.members||
     std::find(ids.begin(),ids.end(),selected.id)==ids.end()||!no_bail_skater(owner)||owner!=b.owner||
     !read_surface_self(context.client_context,owner,self)||self!=b.self||context.self_entity!=self.entity)return false;
  alignas(16) std::array<std::uint64_t,3> query{};std::uint32_t realm{},record{};
  if(traffic_query(query.data(),selected.id)!=query.data()||query!=c.query||!memory::read(query[1]+8,realm)||realm!=std::uint32_t(selected.id>>32)||
     !memory::read(query[2],record)||record!=std::uint32_t(selected.id))return false;
  const auto frame=surface_geometry_native_frame(now);const auto native=InputNativeContext::from_frame(frame);
  return frame.valid&&context.self_generation==frame.owner_generation&&context.native.fresh(now)&&native.fresh(now)&&frame.owner==NativeInputOwner{owner.base,owner.entity,owner.core,owner.context,owner.rig}&&
   context.native.generations==native.generations&&context.native.air_episode()==native.air_episode()&&context.native.airborne==native.airborne&&
   native.physics_step>=context.native.physics_step&&native.sampled_at>=context.native.sampled_at&&population_surface_current(selected);
 }catch(...){return false;}
}
bool traffic_geometry_current(void* user,const SurfaceGeometryContext& context)noexcept{
 return traffic_callback_current(user,context,static_cast<TrafficCallback*>(user)->selected);
}
}
void tick_arcade_traffic_surface(std::uintptr_t park) noexcept {
 PreserveError preserve;invalidate_traffic_surface();
 try{
  auto& local=local_runtime();std::unique_lock native(local.native_mutex);auto& world=world_layers_runtime();std::unique_lock lifetime(world.lifetime_mutex);
  auto& r=traffic_runtime();if(r.observing)return;
  struct ObservationPhase {bool& active;explicit ObservationPhase(bool& value):active(value){active=true;}~ObservationPhase(){active=false;}} observation{r.observing};
  const auto fail=[&]{r.provider.invalidate();r.selected={};r.query={};invalidate_population_surface();};
  const auto now=GetTickCount64();const auto gate=surface_client_gate;TrafficScope scope;
  if(!enabled()||!gate.client||gate.base!=local.base||now<gate.sampled_at){fail();return;}
  // Expiry revokes the copied observation above, not the selected material
  // point's identity. Renewal below must revalidate every owner and binding;
  // the motion estimator independently warms an actual pose gap.
  if(now-gate.sampled_at>150){
   if(r.selected.generation&&!population_surface_current(r.selected))fail();
   return;
  }
  if(!traffic_scope(gate.client,park,scope)||scope.client_context!=gate.context){fail();return;}
  const auto frame=surface_geometry_native_frame(now);NoBailSkater owner{};SurfaceSelf self;
  if(!frame.valid||!no_bail_skater(owner)||frame.owner!=NativeInputOwner{owner.base,owner.entity,owner.core,owner.context,owner.rig}||
     !read_surface_self(scope.client_context,owner,self)){fail();return;}
  const auto& e=park_editor_detail::editor_state();std::vector<std::uint64_t> members;
  if(!e.surface_api.ready||!e.picking_api.ready||!e.picking_api.mapping||!e.picking_api.map_entity||!traffic_members(scope.traffic,members)){fail();return;}
  // Observe only the already queried body. Direct or existing ready-table mapping
  // must independently resolve to authoritative traffic membership. No nearest
  // target search, ancestry guess, player support or car-grab selection is added.
  const auto measured=surface_geometry_observation(now);
  if(!measured.valid){fail();return;}
  auto id=measured.body_owner;
  if(std::find(members.begin(),members.end(),id)==members.end())id=traffic_mapped(id);
  if(!(id>>32)||std::find(members.begin(),members.end(),id)==members.end()){fail();return;}
  alignas(16) std::array<std::uint64_t,3> query{};
  if(traffic_query(query.data(),id)!=query.data()||query[0]!=id||!query[1]||!query[2]){fail();return;}
  const bool same=r.scope==scope&&r.selected.id==id&&r.query==query&&r.body==measured.body&&population_surface_current(r.selected);
  if(!same){r.provider.invalidate();invalidate_population_surface();}
  const auto selected=select_population_surface(id);if(!selected.generation){fail();return;}
  r.scope=scope;r.selected=selected;r.query=query;r.body=measured.body;
  SurfaceGeometryContext context{{owner.base,owner.entity,owner.core,owner.context,owner.rig},InputNativeContext::from_frame(frame),
   scope.client_context,measured.context.world,world.root_generation,self.entity,frame.owner_generation,SurfaceGeometryPhase::park};
  TrafficCallback callback{{native,lifetime,GetCurrentThreadId(),gate.client,owner,self,context,e.surface_api,e.picking_api,true},scope,members,selected,query,placements_runtime().valid,placements_runtime().query,placements_runtime().pose};
  TrafficSurfaceNativeApi api;api.base=local.base;api.ready=true;api.valid=&traffic_valid;api.query=&traffic_query;api.pose=&traffic_pose;
  api.body_valid=&traffic_body_valid;api.body_entity=&traffic_body_entity;api.mapping_ready=true;api.mapping=&traffic_mapping;api.map_entity=&traffic_map;
  api.read_bytes=&memory::read_bytes;api.user=&callback;api.seconds=&traffic_seconds;api.milliseconds=[](void*)noexcept->std::uint64_t{return GetTickCount64();};api.current=&traffic_callback_current;
  if(!traffic_callback_current(&callback,context,selected)){fail();return;}
  if(!r.provider.surface_serial()){
   const auto pose=capture_traffic_surface_pose(api,context,selected);
   SurfaceGeometryNativeApi geometry;geometry.base=local.base;geometry.surface=e.surface_api;
   geometry.picking={e.picking_api.hit_body,&traffic_body_valid,&traffic_body_entity,true};geometry.user=&callback;
   geometry.now=api.milliseconds;geometry.current=&traffic_geometry_current;geometry.read_bytes=&memory::read_bytes;
   // Re-query the same measured footprint after the selected pose capture,
   // preserving the source25ms acquisition budget and actual body token.
   SurfaceGeometryRequest request;constexpr std::array<std::array<float,2>,5> offsets{{{0,0},{-.25f,0},{.25f,0},{0,-.25f},{0,.25f}}};
   for(std::size_t i=0;i<5;++i)request.segments[i]={{measured.point[0]+offsets[i][0],measured.point[1]+.75f,measured.point[2]+offsets[i][1]},
    {measured.point[0]+offsets[i][0],measured.point[1]-2.f,measured.point[2]+offsets[i][1]}};
   const auto queried=query_surface_geometry(geometry,context,request);
   if(!pose.valid||!queried.valid||queried.body!=measured.body||!r.provider.bind(api,pose,queried)){fail();return;}
  }
  const auto result=r.provider.observe(api,context,selected);
  if(!traffic_callback_current(&callback,context,selected)){fail();return;}
  publish_traffic_surface(result);
  if(!population_surface_current(selected))invalidate_traffic_surface();
  callback.base.entered=false;
 }catch(...){traffic_runtime().provider.invalidate();invalidate_population_surface();invalidate_traffic_surface();}
}

}
