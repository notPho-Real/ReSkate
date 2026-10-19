#include "Engine/Core/Log/logging.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Extension/ArcadeMode/arcade_mode.h"
#include "Extension/Profile/runtime_internal.h"
#include "local_population_controls.h"
#include "local_world_controls.h"
#include "Engine/Game/Build/20260929/population.h"

namespace dingosdk::profile_runtime {
PopulationRuntime& population_runtime() { static auto* r = new PopulationRuntime; return *r; }

// Published removal ABI/fingerprint from davad00/ReSkate (GPL-3.0),
// 992e35fddb5bcef914e2ec20b13e9316393b6763, traffic_vehicle_provider.cpp.
namespace {
constexpr std::uintptr_t surface_removal_rva=0x329f640;
constexpr std::array<unsigned char,32> surface_removal_bytes{
 0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x55,0x48,0x8d,0x6c,0x24,0xa9,
 0x48,0x81,0xec,0xb0,0x00,0x00,0x00,0x48,0x8b,0x05,0x62,0x4d,0xf2,0x03,0x48,0x33};
bool advance_surface_counter(std::atomic<std::uint64_t>& counter) noexcept {
 auto value=counter.load(std::memory_order_acquire);
 while(value!=UINT64_MAX){if(counter.compare_exchange_weak(value,value+1,std::memory_order_acq_rel))return true;}
 population_runtime().surface_exhausted.store(true,std::memory_order_release);return false;
}
void population_surface_remove(std::uintptr_t manager,std::uint64_t id) {
 const auto incoming=GetLastError();auto& p=population_runtime();
 p.surface_removals_active.fetch_add(1,std::memory_order_acq_rel);
 const bool selected=id&&p.surface_selected.load(std::memory_order_acquire)==id;
 if(selected){advance_surface_counter(p.surface_revision);arcade_mode::invalidate_traffic_surface();}
 SetLastError(incoming);p.surface_remove(manager,id);const auto returned=GetLastError();
 if(id&&p.surface_selected.load(std::memory_order_acquire)==id){advance_surface_counter(p.surface_revision);arcade_mode::invalidate_traffic_surface();}
 p.surface_removals_active.fetch_sub(1,std::memory_order_acq_rel);SetLastError(returned);
}
}
void invalidate_population_surface() noexcept {
 auto& p=population_runtime();p.surface_selected.store(0,std::memory_order_release);
 advance_surface_counter(p.surface_generation);arcade_mode::invalidate_traffic_surface();
}
arcade_mode::TrafficSurfaceSelection select_population_surface(std::uint64_t id) noexcept {
 auto& p=population_runtime();
 if(!(id>>32)||!p.surface_removal_ready.load(std::memory_order_acquire)||p.surface_exhausted.load(std::memory_order_acquire)||
    p.surface_removals_active.load(std::memory_order_acquire))return {};
 // The caller holds native_mutex and has already proved traffic membership.
 if(p.surface_selected.load(std::memory_order_acquire)!=id){
  invalidate_population_surface();p.surface_selected.store(id,std::memory_order_release);
 }
 arcade_mode::TrafficSurfaceSelection out{id,p.surface_generation.load(std::memory_order_acquire),p.surface_revision.load(std::memory_order_acquire),true};
 return population_surface_current(out)?out:arcade_mode::TrafficSurfaceSelection{};
}
bool population_surface_current(arcade_mode::TrafficSurfaceSelection s) noexcept {
 auto& p=population_runtime();
 return s.removal_free&&(s.id>>32)&&s.generation&&p.surface_removal_ready.load(std::memory_order_acquire)&&
  !p.surface_exhausted.load(std::memory_order_acquire)&&!p.surface_removals_active.load(std::memory_order_acquire)&&
  p.surface_selected.load(std::memory_order_acquire)==s.id&&p.surface_generation.load(std::memory_order_acquire)==s.generation&&
  p.surface_revision.load(std::memory_order_acquire)==s.removals&&!p.surface_removals_active.load(std::memory_order_acquire);
}
void start_population_surface_observer() noexcept {
 auto& p=population_runtime();if(p.surface_removal_ready.load(std::memory_order_acquire))return;
 try{
  const auto base=local_runtime().base;std::array<unsigned char,32> bytes{};
  if(!base||!memory::read_bytes(base+surface_removal_rva,bytes.data(),bytes.size())||bytes!=surface_removal_bytes)return;
  auto* target=reinterpret_cast<void*>(base+surface_removal_rva);
  if(hook_prepare(target,reinterpret_cast<void*>(&population_surface_remove),reinterpret_cast<void**>(&p.surface_remove))!=HookOk)return;
  if(hook_enable(target)!=HookOk)return;
  p.surface_removal_ready.store(true,std::memory_order_release);
 }catch(...){invalidate_population_surface();}
}

bool population_identity(std::uintptr_t manager, int realm) {
    std::uintptr_t vt{}, global{}; int actual{};
    // Every population tick: the game's own manager global first, then that live object, peeked.
    return realm >= 0 && realm <= 1 && manager &&
        memory::peek(local_runtime().base + addr::population::managers + realm * 8, global) && global == manager &&
        memory::peek(manager, vt) && vt == local_runtime().base + addr::population::manager_vtable &&
        memory::peek(manager + 8, actual) && actual == realm;
}

bool population_config_identity(std::uintptr_t config) {
    std::uintptr_t type{}; std::string name;
    // The validated manager supplies a TrafficSystemConfig, but the asset name
    // varies by map (BAM uses Gameplay/AI/BAM_TrafficSystemConfig). The exact
    // native class identifies the field layout; the default asset name does not.
    return memory::peek(config + 8, type) && type == local_runtime().base + addr::population::traffic_system_config_type &&
        identifier(reinterpret_cast<void*>(config + 0x18), name);
}

// Every population tick, on the validated live config: peeked.
bool population_value(std::uintptr_t config, unsigned i, float& out) {
    if (i == 0 || i == 3) return memory::peek(config + population_offsets[i], out) && std::isfinite(out) && out >= 0 && out <= 100;
    std::uint32_t v{}; if (!memory::peek(config + population_offsets[i], v) || v > 1000) return false; out = static_cast<float>(v); return true;
}

bool population_write(std::uintptr_t config, unsigned i, float value) {
    const auto integer = static_cast<std::uint32_t>(value);
    const void* data = i == 0 || i == 3 ? static_cast<const void*>(&value) : &integer;
    SIZE_T written{};
    return WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(config + population_offsets[i]), data, 4, &written) && written == 4;
}

void population_release_config(std::uintptr_t manager) {
    auto& p = population_runtime();
    const auto it = p.nodes.find(manager); if (it == p.nodes.end()) return;
    const auto config = it->second.config; it->second.config = 0;
    if (!config || std::any_of(p.nodes.begin(), p.nodes.end(), [&](const auto& n) { return n.second.config == config; })) return;
    const auto saved = p.configs.find(config);
    if (saved != p.configs.end() && population_config_identity(config)) {
        for (unsigned i = 0; i < population_offsets.size(); ++i) {
            float current{}; const auto& f = saved->second.fields[i];
            if (f.owned && population_value(config, i, current) && current == f.applied) population_write(config, i, f.original);
        }
    }
    p.configs.erase(config);
}

std::uintptr_t population_construct(std::uintptr_t manager, int realm, std::uintptr_t allocator) {
    invalidate_population_surface();
    auto& p = population_runtime(); const auto result = p.construct(manager, realm, allocator);
    PreserveError preserve;
    try {
        std::lock_guard lock(world_control_runtime().mutex);
        if (population_identity(manager, realm)) p.nodes[manager] = PopulationNode{0,realm};
    } catch (...) { dingosdk::logging::event(dingosdk::logging::Channel::world, "{\"event\":\"world_population_capture_failed\"}"); }
    return result;
}

std::uintptr_t population_destroy(std::uintptr_t manager, unsigned flags) {
    invalidate_population_surface();
    auto& p = population_runtime();
    { PreserveError preserve; std::lock_guard lock(world_control_runtime().mutex);
      population_release_config(manager); p.nodes.erase(manager); }
    return p.destroy(manager, flags);
}

int population_choice(unsigned kind) {
    const auto& r = world_control_runtime();
    if (r.map == WorldMap::none || !r.population_active.load(std::memory_order_acquire)) return -1;
    return kind == 0 ? r.model.choices.traffic : r.model.choices.pedestrians;
}

void update_population_controls() {
    auto& r = world_control_runtime(); auto& p = population_runtime(); std::lock_guard lock(r.mutex);
    if (!r.population_active.load(std::memory_order_acquire)) return;
    for (auto& [manager, node] : p.nodes) {
        if (!population_identity(manager, node.realm)) continue;
        for (unsigned kind = 0; kind < 2; ++kind) {
            const auto choice = population_choice(kind);
            if (choice < 0 && !node.owned[kind]) continue;
            std::uint8_t current{};
            if (!read(manager + 0x150 + kind, current) || current > 1) continue;
            if (!node.owned[kind]) node.original[kind] = current != 0;
            const bool desired = choice < 0 ? node.original[kind] : node.realm == 1 || choice != 0;
            p.enable[kind](desired, node.realm);
            node.owned[kind] = choice >= 0;
        }
    }
    if (r.diagnostic_due && GetTickCount64() >= r.diagnostic_due) {
        r.diagnostic_due = 0;
        dingosdk::Json managers = dingosdk::Json::array();
        for (const auto& [manager, node] : p.nodes) {
            if (!population_identity(manager, node.realm)) continue;
            std::array<std::uint8_t,3> flags{}; std::uintptr_t config{};
            read(manager + 0x150, flags); read(manager + 0x108, config);
            config &= ~std::uintptr_t{4};
            std::string name;
            if (population_config_identity(config)) identifier(reinterpret_cast<void*>(config + 0x18), name);
            managers.push_back({{"realm",node.realm},{"flags",flags},{"config",config},{"asset",name}});
        }
        auto fields = dingosdk::Json::array();
        for (unsigned i=0;i<atmosphere_controls.size();++i) {
            const auto& control=atmosphere_controls[i];
            const auto& reading=r.model.atmosphere[i];
            const auto choice=r.model.choices.atmosphere.find(control.key);
            if (choice==r.model.choices.atmosphere.end()) continue;
            fields.push_back({{"control",control.key},{"requested",atmosphere_value_text(control,choice->second)},
                {"observed",reading.value ? atmosphere_value_text(control,*reading.value) : "unavailable"},
                {"varies",reading.varies},{"applied",reading.applied}});
        }
        dingosdk::logging::event(dingosdk::logging::Channel::world, dingosdk::Json{{"event","world_controls_observed"},{"environments",r.model.environments},{"fields",fields},
            {"components",r.nodes.size()},{"population_ready",r.model.population_ready},
            {"population",r.model.population},{"managers",managers},{"status",r.model.status}}.dump().c_str());
    }
}
}