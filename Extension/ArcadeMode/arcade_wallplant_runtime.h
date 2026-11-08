#pragma once
#include "arcade_wallplant.h"
#include "arcade_surface_geometry_native.h"
#include "arcade_input_dispatch.h"
#include "arcade_wallplant_geometry_admission.h"

namespace dingosdk::arcade_mode::wallplant_runtime {
void tick(std::uintptr_t base,bool gameplay) noexcept;
void observe_physics(const NativeInputFrame&) noexcept;
void publish_geometry(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,
    const NativeInputFrame&,SurfaceGeometryVector root,const SurfaceGeometrySample& ground) noexcept;
InputSurfaceState surface(std::uint64_t now,const NativeInputFrame&) noexcept;
InputDisposition request(const InputRequest&,std::uint64_t now,const NativeInputFrame&) noexcept;
WallplantCommand consume(std::uint64_t now,const NativeInputFrame&,SurfaceGeometryVector velocity,
    std::uint64_t expected_input=0,const WallplantReconstructionProof* reconstruction=nullptr) noexcept;
bool current(const WallplantCommand&,std::uint64_t now,const NativeInputFrame&,SurfaceGeometryVector velocity) noexcept;
void completed(const WallplantCommand&,bool applied) noexcept;
WallplantCommand latest_completed() noexcept;
// Read-only pending-work receipt. A zero token avoids runtime locking and all
// native reads/logging on ordinary physics updates; it grants no write rights.
struct WriterProbe {
    std::uint64_t input{},contact{},raw{};
    NativeInputFrame native{};
};
WriterProbe writer_probe() noexcept;
// Diagnostic scope only: a retained collision can be observed before any input
// request exists. Never use this snapshot as command or selection authority.
WriterProbe retained_writer_probe() noexcept;
void retained_writer_event(const WriterProbe&,unsigned stage,const char* reason,
    std::uint32_t state,bool state_known,std::uintptr_t callback_core) noexcept;
void writer_event(const WriterProbe&,unsigned stage,const char* reason,
    std::uint32_t state,bool state_known,std::uintptr_t callback_core) noexcept;
// Retained-contact diagnostics only. This snapshot grants no geometry or input
// admission; events are ignored after its actual collision identity changes.
struct GeometryProbe {
    std::uint64_t contact{},raw{};
    NativeInputFrame native{};
};
GeometryProbe geometry_probe() noexcept;
bool geometry_probe_current(const GeometryProbe&,const NativeInputFrame&) noexcept;
void geometry_event(const GeometryProbe&,unsigned stage,const char* reason) noexcept;
// XInput/physics callers only signal; animation API remains client-thread-only.
void interrupt_animation() noexcept;
void cancel() noexcept;
}
