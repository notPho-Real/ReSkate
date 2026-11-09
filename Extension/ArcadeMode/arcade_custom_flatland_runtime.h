#pragma once
#include "arcade_custom_flatland.h"

namespace dingosdk::arcade_mode {
struct CustomFlatlandBinding { bool enabled{}; unsigned branch{}; bool ready{}; };
namespace custom_flatland_runtime {
// Client thread performs animation calls; input/physics only touch the mailbox.
void tick(CustomFlatlandContext) noexcept;
CustomFlatlandBinding observe(CustomFlatlandContext) noexcept;
InputDisposition request(const InputRequest&) noexcept;
CustomFlatland::Motion consume(const NativeInputFrame&,std::uint64_t now,
    const std::array<float,3>& velocity,bool manual_admitted) noexcept;
void cancel() noexcept;
}
}
