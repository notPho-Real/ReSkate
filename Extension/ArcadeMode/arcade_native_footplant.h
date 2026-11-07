#pragma once
#include "arcade_footplant.h"
#include "Extension/Skater/no_bail.h"
namespace dingosdk::arcade_mode::native_footplant {
void tick(std::uintptr_t image,bool gameplay) noexcept;
bool available() noexcept;
bool request(const NoBailSkater&,std::uint64_t step,Footplant) noexcept;
FootplantControl controls(const NoBailSkater&,std::uint64_t step) noexcept;
void cancel() noexcept;
}
