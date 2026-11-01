#pragma once
#include "Extension/Skater/no_bail.h"
#include "arcade_grind_transition_lifetime.h"
#include "arcade_grind_contact.h"
#include "arcade_grind_admission.h"
#include <cstdint>

namespace dingosdk::arcade_mode::native_grind {
// Native actor lifetime/admission support. Arming this scope does not supply a
// named control recipe or acknowledge a grind; those require independent input
// delivery and physics/animation evidence from the request executor.
bool start(std::uintptr_t base) noexcept;
void tick(bool gameplay) noexcept;
void cancel() noexcept;
bool available() noexcept;
NativeGrindContact contact(const NoBailSkater& owner) noexcept;
bool arm(const NoBailSkater& owner,const NativeGrindContact& expected,std::uint64_t serial,
    std::uint64_t now,std::uint64_t deadline,GrindAdmissionControl control) noexcept;
bool pending() noexcept;
bool setting_override(const void* reference,std::uintptr_t expression) noexcept;
struct Completion {
    NativeGrindCompletion native{};
    std::uint64_t serial{},completed_at{};
};
// Fresh original-actor completion, separate from destination identity/contact.
// An executor must verify those independently before acknowledging a name.
Completion completion(const NoBailSkater& owner,std::uint64_t serial,std::uint64_t now) noexcept;
}
