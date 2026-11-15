#pragma once
#include "Engine/Resource/ebx_document.h"
namespace dingosdk::score_line {
void register_multiplier_label_pack();
// Extend the installed native widget; no game asset bytes ship in the SDK.
[[nodiscard]] frostbite::ebx::Document build_multiplier_label(
    const frostbite::ebx::Document& multiplier, const frostbite::ebx::Document& score_widget,
    const frostbite::ebx::Document& score_model, const frostbite::ebx::Document& label_widget);
}
