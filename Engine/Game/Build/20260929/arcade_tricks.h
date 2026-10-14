#pragma once
#include "skater_body.h"

namespace dingosdk::game::build::v20260929::arcade_tricks {
// Read-only selection used by the upstream Style Editor at revision
// f6d7e5a929f9e2b046ea6b1192b1fac2487079c7. No new executable patch.
// The caller must validate the local No Bail owner chain before and after reads.
inline constexpr auto core_selection = skater_body::core_step_input_offset;
inline constexpr std::uintptr_t selection_flip = 0x28; // int32 FlipTrickType; -1 means none
}
