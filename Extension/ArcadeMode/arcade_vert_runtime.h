#pragma once
#include "arcade_vert_air.h"
#include "arcade_vert_trigger.h"
#include "arcade_input_dispatch.h"
#include "arcade_surface_geometry_native.h"
namespace dingosdk::arcade_mode::vert_runtime {
void cancel() noexcept;
// Successful native AIR submission and completed physical write only.
bool submitted_native_level_out(const NativeInputFrame&,std::uint64_t now,const VertPlan&) noexcept;
TriggerDecision qualify(void*,const TriggerPress&,const InputNativeContext&,std::uint64_t now) noexcept;
InputSurfaceState surface(std::uint64_t now,const NativeInputFrame&,InputSurfaceState other) noexcept;
InputDisposition request(const InputRequest&,std::uint64_t now,const NativeInputFrame&) noexcept;
// Runs in the existing authorized geometry callback; never owns a new hook.
void geometry(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,const NativeInputFrame&,
    SurfaceGeometryVector board,SurfaceGeometryVector up) noexcept;
// Called inside the existing validated physical body writer before staging.
SurfaceGeometryVector alignment(const NativeInputFrame&,std::uint64_t now) noexcept;
struct Motion {VertPlan plan{};bool native_flight{},aligning{},preserve_native{};};
Motion step(VertObservation,std::uint64_t now,const NativeInputOwner& physical_owner,
    const char* frame_reason,const NativeInputCorrelation* correlation=nullptr,
    const NativeRequestSample* live=nullptr) noexcept;
}
