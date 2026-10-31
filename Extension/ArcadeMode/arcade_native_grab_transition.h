#pragma once
#include "Extension/Skater/no_bail.h"
#include "arcade_trick_table.h"
#include "arcade_input_types.h"
#include <cstdint>
namespace dingosdk::arcade_mode::native_grab {
bool start(std::uintptr_t base) noexcept;
void tick(bool gameplay) noexcept;
bool available() noexcept;
bool blocks_flip(const NoBailSkater&) noexcept;
void cancel() noexcept;
void refresh(const NoBailSkater&,std::uint64_t episode,bool held,bool permitted) noexcept;
bool request(const NoBailSkater&,std::uint64_t episode,const InputRequest&,ArcadeTrick,bool held) noexcept;
bool right_hand_input(const NoBailSkater&,std::uint64_t episode) noexcept;
// Bounded, capture-only observation; no native state or admission changes.
void trace_input(std::uint64_t,const UnifiedInput::Diagnostic&,const ActionInputOutput&) noexcept;
// Additive call from the already-qualified StateFlow candidate entry. It does
// not change candidates, PTM selection, animation IDs, or native eligibility.
void before_transition(std::uintptr_t view,float delta) noexcept;
}
