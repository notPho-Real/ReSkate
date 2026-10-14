#pragma once
#include <cstdint>
namespace dingosdk::arcade_mode {
// Client-thread read-only discovery of the native scoring HUD schema. It never
// publishes UI values or logs arbitrary game strings, pointers or user data.
void observe_score_hud(std::uintptr_t base, std::uint64_t now) noexcept;
}
