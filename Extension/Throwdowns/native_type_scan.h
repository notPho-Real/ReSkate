#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace dingosdk::multiplayer {
// A data-defined (EBX) type wanted by its runtime hash (== EBX TypeNameHash for events) and
// type kind (record flags bits 5-9: 2 = class, 24 = script function).
struct NativeTypeQuery { std::uint32_t hash{}; std::uintptr_t object{}; std::uint8_t kind = 2; };
// Fills `object` for every query it finds on the heap and returns how many are still
// missing. ~0.15 s in the world; never call it during level load (it then finds nothing
// and takes minutes). background: for a worker thread only; the scan runs at background
// priority, yields between regions and skips pages outside the working set.
std::size_t find_native_types(std::span<NativeTypeQuery> wanted, bool background = false) noexcept;
// Worker-only incremental alternative. Each call visits at most region_budget
// regions and byte_budget resident-page candidates (rounded down to 4 KiB).
// A failed working-set query is skipped, never faulted in. Callers may cancel
// between steps by discarding the cursor and query objects. No partial binding
// should be exposed to game threads. Existing full-scan callers are unchanged.
struct NativeTypeScanCursor { std::uintptr_t next{0x10000}; bool complete{}; };
std::size_t find_native_types_step(std::span<NativeTypeQuery> wanted,
    NativeTypeScanCursor& cursor, std::size_t byte_budget, unsigned region_budget) noexcept;
// Same bounded resident-page traversal for fixed asset-ID readers. Worker only;
// visitor receives one readable4KiB span and returns false when finished.
using NativePageVisitor = bool (*)(void*, std::uintptr_t, std::size_t) noexcept;
void visit_native_pages_step(NativeTypeScanCursor& cursor, std::size_t byte_budget,
    unsigned region_budget, NativePageVisitor visitor, void* context) noexcept;
}
