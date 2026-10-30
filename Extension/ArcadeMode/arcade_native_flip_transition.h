#pragma once
#include "Extension/Skater/no_bail.h"
#include <cstdint>
namespace dingosdk::arcade_mode::native_flip {
bool start(std::uintptr_t base) noexcept;
void tick(bool gameplay) noexcept;
void cancel() noexcept;
bool available() noexcept;
void arm(const NoBailSkater& owner) noexcept;
bool request(const NoBailSkater& owner, std::int32_t id) noexcept;
}
