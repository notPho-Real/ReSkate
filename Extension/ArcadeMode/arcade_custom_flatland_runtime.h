#pragma once
#include "arcade_custom_flatland.h"
#include "arcade_custom_score.h"

namespace dingosdk::arcade_mode {
struct CustomFlatlandBinding { bool enabled{}; unsigned branch{}; bool ready{}; bool owns_triggers{}; };
namespace custom_flatland_runtime {
// Enabled by default; the incoming Arcade context gates all gameplay.
// The unsaved override is retained for development diagnostics.
// Bounded scalar diagnostics, reset by OFF -> ON. These are controller requests,
// not proof that the writer or the renderer applied them.
struct Observation {
    std::array<std::uint64_t,5> surface_steps{};
    std::uint64_t step{};
    float seconds{}, deceleration{}, speed{}, requested_loss{}, display_radians{}, left_seconds{}, right_seconds{};
    FlatlandSurface surface{};
    bool verified{};
};
struct Status {
    bool enabled{}, ready{}, animation_current{};
    bool render_seed{};
    CustomFlatland::Phase phase{};
    Observation observation{};
    // Last accepted entry seed, not proof of a rendered frame.
    std::uint64_t seed_sequence{}, seed_ticket{};
    double seed_age{};
};
CustomExecutionBatch take_executions() noexcept;
void set_enabled(bool) noexcept;
#if defined(DINGOSDK_DEV_TOOLS)
void set_render_seed(bool) noexcept;
#endif
Status status() noexcept;
// Client thread performs animation calls; input/physics only touch the mailbox.
void tick(CustomFlatlandContext) noexcept;
CustomFlatlandBinding observe(CustomFlatlandContext) noexcept;
CustomFlatlandBinding observe_input(CustomFlatlandContext, unsigned left_trigger, unsigned right_trigger) noexcept;
InputDisposition request(const InputRequest&) noexcept;
CustomFlatland::Motion consume(const NativeInputFrame&,std::uint64_t now,
    const std::array<float,3>& velocity,bool manual_admitted,const FlatlandSurfaceEvidence& surface={}) noexcept;
void cancel() noexcept;
}
}
