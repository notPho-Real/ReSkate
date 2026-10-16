#pragma once
#include "arcade_native_score_schema.h"

namespace dingosdk::arcade_mode::native_score_detail {
enum class BinderOutcome : unsigned {
    contract_rejected, mapping_unreadable, count_rejected, layout_rejected,
    value_mismatch, relationship_matches, unverified, stale, count
};
// Independent reasons may coexist. Bits classify the existing three records;
// neither these bits nor the sanitized shape snapshot relax the guards.
enum BinderLayoutFailure : unsigned {
    binder_alignment = 1U, binder_range = 2U, binder_metadata = 4U, binder_duplicate = 8U
};
inline constexpr std::array<unsigned, 4> binder_layout_bits{
    binder_alignment, binder_range, binder_metadata, binder_duplicate};
struct BinderRecord {
    std::uint32_t offset{};
    unsigned metadata_low{};
    bool metadata_high{};
    bool operator==(const BinderRecord&) const = default;
};
struct BinderDiagnostic {
    Route route{Route::none};
    BinderOutcome outcome{BinderOutcome::unverified};
    unsigned layout_failures{};
    std::array<BinderRecord, 3> records{};
    bool records_available{};
};
// These are the existing common binder wrapper/table reads, not guessed scoring
// parameter offsets. No input slot, Context, runtime instance or payload is read.
template<class Read, class Current>
BinderDiagnostic inspect_score_binder(Read& read, std::uintptr_t serialized, Current&& current) noexcept {
    const auto range = [](std::uintptr_t address, std::size_t size) {
        constexpr auto highest = std::uintptr_t{0x00007fffffff0000ULL};
        return address >= 0x10000 && size <= highest && address <= highest - size;
    };
    BinderDiagnostic result{};
    if (!current() || !range(serialized, 0x60)) return result;
    std::uintptr_t resource{}; std::uint32_t hash{};
    if (!read(serialized + 0x38, resource) || !range(resource, 0x30) || !read(resource + 0x10, hash)) return result;
    const auto* graph = graph_for(hash);
    if (!graph) return result;
    result.route = graph->route;
    std::uint32_t frame{}, constants{}, words{};
    if (!read(resource + 0x20, frame) || !read(resource + 0x24, constants) || !read(resource + 0x2c, words) ||
        !contract(*graph, frame, constants, words)) {
        result.outcome = BinderOutcome::contract_rejected;
    } else if (graph->route == Route::line_start || graph->route == Route::stats_line_end) {
        // Start has no independently mapped Value slot; the stats handler's
        // declaration parameter list has not been identified. Do not invent it.
        result.outcome = BinderOutcome::unverified;
    } else {
        struct Mapping { std::uint32_t register_offset{}, native_metadata{}; };
        static_assert(sizeof(Mapping) == 8);
        std::uintptr_t table{}; std::uint32_t encoded_count{};
        std::array<Mapping, 3> mappings{};
        if (!read(serialized + 0x58, table) || !range(table, sizeof(mappings)) ||
            !range(table - 4, 4) || !read(table - 4, encoded_count)) {
            result.outcome = BinderOutcome::mapping_unreadable;
        } else if ((encoded_count & 0x7fffffffU) != mappings.size()) {
            result.outcome = BinderOutcome::count_rejected;
        } else if (!read(table, mappings)) {
            result.outcome = BinderOutcome::mapping_unreadable;
        } else {
            result.records_available = true;
            for (unsigned i = 0; i < mappings.size(); ++i) {
                const auto& mapping = mappings[i];
                result.records[i] = {mapping.register_offset, mapping.native_metadata & 0xffU,
                    (mapping.native_metadata & 0xffffff00U) != 0};
                if (mapping.register_offset & 7U) result.layout_failures |= binder_alignment;
                if (mapping.register_offset > frame || frame - mapping.register_offset < 8)
                    result.layout_failures |= binder_range;
                if (mapping.native_metadata & 0xffU) result.layout_failures |= binder_metadata;
                for (unsigned prior = 0; prior < i; ++prior)
                    if (mappings[prior].register_offset == mapping.register_offset)
                        result.layout_failures |= binder_duplicate;
            }
            result.outcome = result.layout_failures ? BinderOutcome::layout_rejected :
                mappings[2].register_offset == graph->input ? BinderOutcome::relationship_matches : BinderOutcome::value_mismatch;
        }
    }
    // Relationship match describes static mapping compatibility only. It proves
    // neither parameter representation nor ownership nor native occurrence ID.
    if (!current()) {
        result.outcome = BinderOutcome::stale; result.layout_failures = 0;
        result.records_available = false; result.records = {};
    }
    return result;
}
}
