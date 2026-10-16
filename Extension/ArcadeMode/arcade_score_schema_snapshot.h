#pragma once
#include "arcade_native_score_schema.h"

namespace dingosdk::arcade_mode::native_score_detail {
enum class SchemaSnapshotStatus : unsigned { complete, inactive, type_changed, invalid_fields, fields_changed };
struct SchemaMember {
    std::uint32_t hash{}, type_hash{};
    unsigned kind{}, size{};
    bool operator==(const SchemaMember&) const = default;
};
// Only declaration metadata crosses this boundary; no address, offset, string,
// or payload value is retained in the report.
struct SchemaTypeSnapshot {
    std::uint32_t hash{};
    unsigned size{}, count{};
    std::array<SchemaMember, 96> members{};
    bool operator==(const SchemaTypeSnapshot&) const = default;
};
struct SchemaSnapshot {
    SchemaSnapshotStatus status{SchemaSnapshotStatus::inactive};
    std::array<SchemaTypeSnapshot, 2> types{};
};
template<class Read> bool snapshot_fields(Read& read, const Type& type, std::uint32_t hash,
                                        SchemaTypeSnapshot& output) noexcept {
    SchemaTypeSnapshot result{}; result.hash = hash; result.size = type.size;
    std::uint16_t count{}; std::uintptr_t table{};
    if (!read(type.record + 0x2a, count) || !count || count > result.members.size() ||
        !read(type.record + 0x60, table) || !table || table > UINTPTR_MAX - count * 24ULL) return false;
    result.count = count;
    for (unsigned i = 0; i < count; ++i) {
        auto& row = result.members[i]; Field member{}; std::uintptr_t record{};
        if (!read(table + i * 24ULL, row.hash)) return false;
        for (unsigned prior = 0; prior < i; ++prior)
            if (result.members[prior].hash == row.hash) return false;
        if (!field(read, type, row.hash, member) || member.kind == 0 || member.kind > 28 ||
            !read(member.type, record) || !read(record, row.type_hash)) return false;
        row.kind = member.kind; row.size = member.size;
    }
    output = result; return true;
}
template<class Read, class Current>
SchemaSnapshot snapshot_score_schema(Read& read, const Binding& binding, Current&& current) noexcept {
    SchemaSnapshot result{};
    if (!current()) return result;
    const auto identities_match = [&] {
        for (unsigned i = 0; i < binding.types.size(); ++i)
            if (!type_matches(read, binding.types[i], type_hashes[i])) return false;
        return true;
    };
    if (!identities_match()) { result.status = SchemaSnapshotStatus::type_changed; return result; }
    std::array<SchemaTypeSnapshot, 2> first{}, second{};
    bool valid = true;
    for (unsigned i = 0; i < first.size(); ++i) {
        const auto source = 1 + i * 2;
        valid = snapshot_fields(read, binding.types[source], type_hashes[source], first[i]) && valid;
        valid = snapshot_fields(read, binding.types[source], type_hashes[source], second[i]) && valid;
    }
    if (!current()) return result;
    if (!identities_match()) { result.status = SchemaSnapshotStatus::type_changed; return result; }
    if (!current()) return result;
    if (!valid) { result.status = SchemaSnapshotStatus::invalid_fields; return result; }
    if (first != second) { result.status = SchemaSnapshotStatus::fields_changed; return result; }
    result.types = first; result.status = SchemaSnapshotStatus::complete; return result;
}
// Worker-owned lifetime budget: resets do not replay an unchanged type set.
// At most eight sets, three attempts each, including incomplete attempts.
class SchemaSnapshotGate {
    struct Entry { std::array<Type, 2> types{}; unsigned attempts{}; bool complete{}; };
    std::array<Entry, 8> entries_{};
    unsigned count_{};
public:
    static constexpr unsigned denied = 8;
    unsigned begin(const Binding& binding) noexcept {
        const std::array<Type, 2> types{binding.types[1], binding.types[3]};
        unsigned index{};
        for (; index < count_; ++index) {
            bool same = true;
            for (unsigned i = 0; i < types.size(); ++i) {
                const auto& a = types[i]; const auto& b = entries_[index].types[i];
                same = same && a.object == b.object && a.record == b.record && a.size == b.size;
            }
            if (same) break;
        }
        if (index == count_) {
            if (count_ == entries_.size()) return denied;
            entries_[count_++].types = types;
        }
        auto& entry = entries_[index];
        if (entry.complete || entry.attempts == 3) return denied;
        ++entry.attempts; return index;
    }
    void finish(unsigned index, SchemaSnapshotStatus status) noexcept {
        if (index < count_ && status == SchemaSnapshotStatus::complete) entries_[index].complete = true;
    }
};
}
