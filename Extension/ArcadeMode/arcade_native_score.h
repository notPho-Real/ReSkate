#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
// Client tick: expire/rebind on local context changes and start worker-only
// discovery/draining of sanitized diagnostic records. No scoring writes or combo
// policy. A client-realm graph is NOT proof that its payload owns our skater.
void native_score_tick(std::uintptr_t image_base, std::uint64_t now, bool gameplay) noexcept;
// Existing interpreter entry hook, before execution, pc == 0 only. Publication
// never waits for a lock, allocates, logs, or performs schema discovery.
void observe_native_score_expression(std::uintptr_t vm, std::uint32_t pc) noexcept;
}
