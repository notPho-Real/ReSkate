#pragma once
#include "arcade_held_flip.h"
#include "Extension/Skater/no_bail.h"
namespace dingosdk::arcade_mode::native_held_flip {
void tick(std::uintptr_t image,bool gameplay) noexcept;
HeldFlipObservation observe(const NoBailSkater&,std::uint64_t now) noexcept;
void initialized(std::uintptr_t actor,std::uintptr_t view) noexcept;
struct Evaluation {std::uintptr_t actor{};float before{};std::uint64_t serial{};};
Evaluation before_evaluate(std::uintptr_t actor,std::uintptr_t context,float delta) noexcept;
void after_evaluate(Evaluation,std::uintptr_t context,float delta) noexcept;
}
