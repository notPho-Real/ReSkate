#pragma once
#include "native_score_snapshot.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// skate.'s score HUD as values (ScoringHUDViewModel, read through the game's UI models), what
// trick_watch.h turns into tricks.
namespace dingosdk::tricks {
struct ScoreHudRow {std::uint64_t handle{};std::string name;};
struct ScoreHud {
    bool action{}; // a trick is going on
    bool slam{};
    float line_score{}, multiplier{};
    // The sequence's header ("Kickflip", "Manual"...) and descriptor labels as the HUD shows them,
    // and its sequence state: empty, or 0, when this build's model does not have them readable.
    std::string header, descriptor;
    std::uint32_t sequence_state{};
    float line_timer{}; // what is left of the line's time to keep its multiplier; 0 with no line
    float line_shown{}; // the widget's LINE number, which stays after the line's action signals end
    // Native presentation identities, newest first. They distinguish repeated
    // names in the display but do not establish performed tricks or score awards.
    std::vector<ScoreHudRow> rows{};
    bool rows_read{};
    // Read from the original HUD's referenced scoring owner. Absence means
    // unreadable, never an empty or zero-point award.
    std::optional<NativeScoreSnapshot> native_scoring{};
};
// Whether the HUD names a manual going on (its header or descriptor says one).
bool names_manual(const ScoreHud& now);
// Client thread. Empty while there is no score HUD model (the menu, a level loading), or when this
// game build's model has none of the fields; the model is looked for at most once a second.
std::optional<ScoreHud> read_score_hud(std::uintptr_t image_base, bool include_native_scoring = false) noexcept;
}
