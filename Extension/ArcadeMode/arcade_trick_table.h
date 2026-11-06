#pragma once
#include "arcade_trick_registry.h"
namespace dingosdk::arcade_mode {
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
