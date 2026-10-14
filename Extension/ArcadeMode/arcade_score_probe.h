#pragma once
#include <cstdint>
#include "arcade_score_event.h"
namespace dingosdk::arcade_mode {
// Client-thread read-only discovery of the native scoring HUD schema. It never
// publishes raw UI text or logs arbitrary game strings, pointers or user data.
void observe_score_hud(std::uintptr_t base, std::uint64_t now) noexcept;
struct ScoreActivity { bool valid{}, active{}; std::uint64_t sampled_at{}; };
// Client-thread snapshot; consumers must enforce freshness and native ownership.
ScoreActivity score_activity() noexcept;
// One client-thread consumer. Only fresh transitions in allowlisted native HUD
// text are returned; baseline/gaps/coalesced edges are explicitly uncertain.
// This is label evidence, not a score amount or proof of an unseen repeated trick.
ScoreLabelOccurrence take_score_label_occurrence(std::uint64_t now) noexcept;
void reset_score_label_occurrences() noexcept;
}
