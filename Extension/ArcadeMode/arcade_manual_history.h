#pragma once
#include "arcade_score_labels.h"
#include <array>
#include <cstddef>
#include <span>

namespace dingosdk::arcade_mode {
// Borrowed identities from the already-read native presenters. Never log these
// rows or retain them outside this bounded, client-thread diagnostic tracker.
struct ManualHistoryRow {
    std::uintptr_t handle{}, type{};
    std::uint32_t label{};
};
struct ManualHistoryContext {
    std::uint64_t now{};
    std::uintptr_t manager{}, model_handle{}, model_type{};
    bool history_source{}, complete{};
};
enum class ManualHistoryStatus : std::uint8_t {
    baseline, complete, gap, clock_reset, model_changed, invalid, fallback
};
// Counts describe UI presentation only. Neither presenter identity nor a count
// increase establishes native occurrence, local ownership or scoring authority.
// available=false means counts are unavailable, not measured zero. Deltas are
// meaningful only when comparable=true; baseline deltas are deliberately absent.
struct ManualHistoryDiagnostic {
    ManualHistoryStatus status{ManualHistoryStatus::invalid};
    bool available{}, comparable{};
    unsigned rows{}, manual_rows{}, nose_rows{}, distinct{}, manual_distinct{}, nose_distinct{};
    unsigned manual_added{}, manual_removed{}, nose_added{}, nose_removed{}, duplicates{}, relabels{};
    bool operator==(const ManualHistoryDiagnostic&) const = default;
};
class ManualHistory {
public:
    static constexpr std::size_t capacity = 16;
    static constexpr std::uint64_t freshness_ms = 150;
    void reset() noexcept { primed_ = false; count_ = 0; previous_ = {}; context_ = {}; }
    ManualHistoryDiagnostic observe(std::span<const ManualHistoryRow> rows,
                                     const ManualHistoryContext& context) noexcept {
        const auto invalid = [&](ManualHistoryStatus status) {
            reset(); return ManualHistoryDiagnostic{status};
        };
        if (!context.history_source) return invalid(ManualHistoryStatus::fallback);
        if (!context.complete || !context.manager || !context.model_handle || !context.model_type ||
            rows.size() > capacity) return invalid(ManualHistoryStatus::invalid);
        ManualHistoryDiagnostic result;
        std::array<ManualHistoryRow, capacity> unique{};
        std::size_t count{};
        for (const auto& row : rows) {
            if (!row.handle || !row.type || row.label > score_labels.size())
                return invalid(ManualHistoryStatus::invalid);
            ++result.rows;
            result.manual_rows += row.label == 1;
            result.nose_rows += row.label == 2;
            const auto prior = find(unique, count, row);
            if (prior < count) {
                // One borrowed presenter cannot have two simultaneous labels in
                // a complete snapshot. Do not publish a partially counted view.
                if (unique[prior].label != row.label) return invalid(ManualHistoryStatus::invalid);
                ++result.duplicates;
            } else {
                unique[count++] = row;
                result.manual_distinct += row.label == 1;
                result.nose_distinct += row.label == 2;
            }
        }
        result.distinct = static_cast<unsigned>(count);
        result.available = true;
        if (!primed_) result.status = ManualHistoryStatus::baseline;
        else if (context.now < context_.now) result.status = ManualHistoryStatus::clock_reset;
        else if (context.manager != context_.manager || context.model_handle != context_.model_handle ||
                 context.model_type != context_.model_type) result.status = ManualHistoryStatus::model_changed;
        else if (context.now - context_.now > freshness_ms) result.status = ManualHistoryStatus::gap;
        else {
            result.status = ManualHistoryStatus::complete;
            result.comparable = true;
            for (std::size_t i = 0; i < count; ++i) {
                const auto prior = find(previous_, count_, unique[i]);
                const bool new_label = prior == count_ || previous_[prior].label != unique[i].label;
                result.manual_added += new_label && unique[i].label == 1;
                result.nose_added += new_label && unique[i].label == 2;
                result.relabels += prior < count_ && previous_[prior].label != unique[i].label;
            }
            for (std::size_t i = 0; i < count_; ++i) {
                const auto current = find(unique, count, previous_[i]);
                const bool old_label = current == count || unique[current].label != previous_[i].label;
                result.manual_removed += old_label && previous_[i].label == 1;
                result.nose_removed += old_label && previous_[i].label == 2;
            }
        }
        previous_ = unique; count_ = count; context_ = context; primed_ = true;
        return result;
    }
private:
    static std::size_t find(const std::array<ManualHistoryRow, capacity>& rows, std::size_t count,
                            const ManualHistoryRow& wanted) noexcept {
        for (std::size_t i = 0; i < count; ++i)
            if (rows[i].handle == wanted.handle && rows[i].type == wanted.type) return i;
        return count;
    }
    std::array<ManualHistoryRow, capacity> previous_{};
    std::size_t count_{};
    ManualHistoryContext context_{};
    bool primed_{};
};
}
