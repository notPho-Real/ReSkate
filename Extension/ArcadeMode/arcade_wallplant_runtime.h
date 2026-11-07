#pragma once
#include "arcade_wallplant.h"
#include "arcade_input_dispatch.h"

namespace dingosdk::arcade_mode { struct SurfaceGeometryNativeApi; }

namespace dingosdk::arcade_mode::wallplant_runtime {
void tick(std::uintptr_t base,bool gameplay) noexcept;
void observe_physics(const NativeInputFrame&) noexcept;
void publish_geometry(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,
    const NativeInputFrame&,SurfaceGeometryVector root,const SurfaceGeometrySample& ground) noexcept;
InputSurfaceState surface(std::uint64_t now,const NativeInputFrame&) noexcept;
InputDisposition request(const InputRequest&,std::uint64_t now,const NativeInputFrame&) noexcept;
WallplantCommand consume(std::uint64_t now,const NativeInputFrame&,SurfaceGeometryVector velocity) noexcept;
bool current(const WallplantCommand&,std::uint64_t now,const NativeInputFrame&,SurfaceGeometryVector velocity) noexcept;
void completed(const WallplantCommand&,bool applied) noexcept;
WallplantCommand latest_completed() noexcept;
// XInput/physics callers only signal; animation API remains client-thread-only.
void interrupt_animation() noexcept;
void cancel() noexcept;
}
