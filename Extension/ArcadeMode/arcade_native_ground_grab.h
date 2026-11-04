#pragma once
#include "arcade_ground_grab.h"
#include "Extension/Skater/no_bail.h"
namespace dingosdk::arcade_mode::native_ground_grab {
bool available() noexcept;
bool request(const NoBailSkater&,std::uint64_t step,GroundGrab) noexcept;
GroundGrabControl controls(const NoBailSkater&,std::uint64_t step,bool held) noexcept;
void cancel() noexcept;
struct Evaluation {std::uint64_t serial{};std::uintptr_t actor{};float before{};};
void initialized(std::uintptr_t actor,std::uintptr_t context) noexcept;
Evaluation before_evaluate(std::uintptr_t actor,std::uintptr_t context,float delta) noexcept;
void after_evaluate(Evaluation,std::uintptr_t context,float delta) noexcept;
}
