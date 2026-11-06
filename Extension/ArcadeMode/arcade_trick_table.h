#pragma once
#include "arcade_trick_registry.h"
namespace dingosdk::arcade_mode {
// Context-selected semantics; these identities do not name native methods.
enum class InputAirTableKind : std::uint8_t { none, grab_extra, pressure, count };
enum class ContextualPredicate : std::uint8_t { press, air_direction };
enum ContextualTrickFlags : std::uint8_t { contextual_extra = 1, contextual_board_rotation = 2, contextual_followup_flip = 4 };
struct ContextualTrickRow {
    InputAirTableKind table{};
    ArcadeTrick trick{};
    TrickFamily family{};
    ContextualPredicate predicate{};
    std::uint16_t query_ms{};
    std::int16_t face{}, direction{};
    std::uint8_t flags{};
    bool operator==(const ContextualTrickRow&) const = default;
};
inline constexpr std::array contextual_trick_table{
    ContextualTrickRow{InputAirTableKind::grab_extra,ArcadeTrick::compact_grab,TrickFamily::grab,
        ContextualPredicate::press,300,13,0,contextual_extra},
    ContextualTrickRow{InputAirTableKind::pressure,ArcadeTrick::pressure_turn_flip,TrickFamily::flip,
        ContextualPredicate::air_direction,300,14,6,contextual_board_rotation|contextual_followup_flip}};
constexpr const ContextualTrickRow* contextual_trick(InputAirTableKind table) noexcept {
    for (const auto& row : contextual_trick_table) if (row.table == table) return &row;
    return nullptr;
}
struct TrickTableRow { TrickFamily family; TrickDirection direction; unsigned presses; ArcadeTrick trick; std::string_view name; };
// Compatibility view, generated from the canonical registry; never edit bindings here.
inline constexpr auto literal_trick_table=[] {
    constexpr auto size=[] {std::size_t n=0;for(const auto& t:trick_registry)for(unsigned i=0;i<t.binding_count;++i)if(t.bindings[i].pattern==BindingPattern::named)++n;return n;}();
    std::array<TrickTableRow,size> result{};std::size_t at=0;
    for(const auto& t:trick_registry)for(unsigned i=0;i<t.binding_count;++i) {
        const auto& b=t.bindings[i];if(b.pattern!=BindingPattern::named)continue;
        const auto family=t.category==TrickCategory::flips?TrickFamily::flip:t.category==TrickCategory::grabs?TrickFamily::grab:TrickFamily::grind;
        result[at++]={family,b.direction,b.presses,b.variant,t.name};
    }
    return result;
}();
constexpr ArcadeTrick select_named_trick(TrickFamily family,TrickDirection direction,unsigned presses) noexcept {
    return select_registry_trick(trick_registry,family,direction,presses);
}
enum class RailRelationship : std::uint8_t { unknown, parallel, perpendicular };
constexpr ArcadeTrick select_grind(TrickDirection direction, unsigned presses,
        unsigned direction_presses, RailRelationship relationship) noexcept {
    if (direction_presses == 2 && presses == 1) {
        if (direction == TrickDirection::up) return ArcadeTrick::nosebluntslide;
        if (direction == TrickDirection::down) return ArcadeTrick::bluntslide;
        return ArcadeTrick::none;
    }
    if (direction_presses != 1) return ArcadeTrick::none;
    if (direction == TrickDirection::none && presses == 1) {
        if (relationship == RailRelationship::parallel) return ArcadeTrick::grind_50_50;
        if (relationship == RailRelationship::perpendicular) return ArcadeTrick::boardslide_or_lipslide;
        return ArcadeTrick::none;
    }
    return select_named_trick(TrickFamily::grind, direction, presses);
}
}
