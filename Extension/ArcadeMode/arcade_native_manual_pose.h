#pragma once
#include <cstdint>
namespace dingosdk::arcade_mode::native_manual_pose {
void tick(std::uintptr_t image,bool gameplay) noexcept;
void initialized(std::uintptr_t actor,std::uintptr_t view) noexcept;
struct Evaluation {std::uintptr_t actor{};float before{};std::uint64_t serial{};unsigned index{};};
Evaluation before_evaluate(std::uintptr_t actor,std::uintptr_t context,float delta) noexcept;
void after_evaluate(Evaluation,std::uintptr_t context,float delta) noexcept;
}
