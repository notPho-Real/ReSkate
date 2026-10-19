#pragma once
#include <cstdint>
#include "arcade_rail_adapter_probe.h"
namespace dingosdk { struct NoBailSkater; }
namespace dingosdk::arcade_mode {
// Read-only diagnostics also run with Arcade OFF for native positive controls.
void grind_observer_tick(std::uintptr_t base, std::uint64_t now, bool gameplay,
    bool arcade_enabled, bool speed_valid, float speed) noexcept;
// Copied read-only fields from this physics observation, never actor readiness.
RailAdapterLipSnapshot observe_grind_step(const NoBailSkater& owner) noexcept;
void observe_grind_pad(unsigned user, std::uint64_t now, bool requested_y,
    short right_x, short right_y, bool arcade_enabled) noexcept;
void invalidate_grind_observer() noexcept;
}
