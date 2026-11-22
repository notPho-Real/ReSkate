#pragma once
#include "arcade_stance_input.h"
#include "Extension/Skater/no_bail.h"

namespace dingosdk::arcade_mode {
struct InputRequest;
namespace native_stance {
void tick(std::uintptr_t image,bool gameplay) noexcept;
bool available() noexcept;
bool request(const NoBailSkater&,const InputRequest&,const InputNativeContext&,std::uint64_t now) noexcept;
StanceControl controls(const NoBailSkater&,const InputNativeContext&,std::uint64_t now,std::uint64_t tick) noexcept;
void delivered(const NoBailSkater&,const InputNativeContext&,StanceControl,std::uint16_t final_buttons,
               std::uint64_t now,std::uint64_t tick) noexcept;
void cancel() noexcept;
}
}
