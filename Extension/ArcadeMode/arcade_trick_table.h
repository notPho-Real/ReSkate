#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace dingosdk::arcade_mode {
// Semantic requests only. A native adapter must separately establish exact support.
enum class TrickDirection : std::uint8_t { none, left, up, right, down, up_left, up_right, down_left, down_right, invalid };
enum class TrickFamily : std::uint8_t { flip, grab, grind };
enum class ArcadeTrick : std::uint8_t {
    none, kickflip, impossible, heelflip, pop_shove_it,
    hardflip, inward_heelflip, varial_kickflip, varial_heelflip,
    double_kickflip, double_impossible, double_heelflip, shove_it_360,
    hardflip_360, inward_heelflip_360, flip_360, heelflip_360,
    triple_kickflip, triple_impossible, triple_heelflip, shove_it_540,
    melon, nosegrab, indy, tailgrab, japan, madonna, benihana, airwalk,
    method, rocket_air, stiffy, one_foot_tailgrab, one_foot_japan, judo, sacktap, christ_air,
    grind_50_50, boardslide_or_lipslide, nosegrind, grind_5_0,
    noseslide_or_tailslide, crooked_or_overcrook, smith_or_feeble,
    nosebluntslide, bluntslide, nosegrind_to_pivot, grind_5_0_overturn,
    hurricane, salad, hang_ten_nosegrind, crail_slide, double_blunt_slide, darkslide
};
struct TrickTableRow {
    TrickFamily family;
    TrickDirection direction;
    unsigned presses;
    ArcadeTrick trick;
    std::string_view name;
};
inline constexpr std::array literal_trick_table{
    TrickTableRow{TrickFamily::flip, TrickDirection::left, 1, ArcadeTrick::kickflip, "Kickflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::up, 1, ArcadeTrick::impossible, "Impossible"},
    TrickTableRow{TrickFamily::flip, TrickDirection::right, 1, ArcadeTrick::heelflip, "Heelflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::down, 1, ArcadeTrick::pop_shove_it, "Pop Shove-It"},
    TrickTableRow{TrickFamily::flip, TrickDirection::up_left, 1, ArcadeTrick::hardflip, "Hardflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::up_right, 1, ArcadeTrick::inward_heelflip, "Inward Heelflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::down_left, 1, ArcadeTrick::varial_kickflip, "Varial Kickflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::down_right, 1, ArcadeTrick::varial_heelflip, "Varial Heelflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::left, 2, ArcadeTrick::double_kickflip, "Double Kickflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::up, 2, ArcadeTrick::double_impossible, "Double Impossible"},
    TrickTableRow{TrickFamily::flip, TrickDirection::right, 2, ArcadeTrick::double_heelflip, "Double Heelflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::down, 2, ArcadeTrick::shove_it_360, "360 Shove-It"},
    TrickTableRow{TrickFamily::flip, TrickDirection::up_left, 2, ArcadeTrick::hardflip_360, "360 Hardflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::up_right, 2, ArcadeTrick::inward_heelflip_360, "360 Inward Heelflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::down_left, 2, ArcadeTrick::flip_360, "360 Flip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::down_right, 2, ArcadeTrick::heelflip_360, "360 Heelflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::left, 3, ArcadeTrick::triple_kickflip, "Triple Kickflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::up, 3, ArcadeTrick::triple_impossible, "Triple Impossible"},
    TrickTableRow{TrickFamily::flip, TrickDirection::right, 3, ArcadeTrick::triple_heelflip, "Triple Heelflip"},
    TrickTableRow{TrickFamily::flip, TrickDirection::down, 3, ArcadeTrick::shove_it_540, "540 Shove-It"},
    TrickTableRow{TrickFamily::grab, TrickDirection::left, 1, ArcadeTrick::melon, "Melon"},
    TrickTableRow{TrickFamily::grab, TrickDirection::up, 1, ArcadeTrick::nosegrab, "Nosegrab"},
    TrickTableRow{TrickFamily::grab, TrickDirection::right, 1, ArcadeTrick::indy, "Indy"},
    TrickTableRow{TrickFamily::grab, TrickDirection::down, 1, ArcadeTrick::tailgrab, "Tailgrab"},
    TrickTableRow{TrickFamily::grab, TrickDirection::up_left, 1, ArcadeTrick::japan, "Japan"},
    TrickTableRow{TrickFamily::grab, TrickDirection::up_right, 1, ArcadeTrick::madonna, "Madonna"},
    TrickTableRow{TrickFamily::grab, TrickDirection::down_left, 1, ArcadeTrick::benihana, "Benihana"},
    TrickTableRow{TrickFamily::grab, TrickDirection::down_right, 1, ArcadeTrick::airwalk, "Airwalk"},
    TrickTableRow{TrickFamily::grab, TrickDirection::left, 2, ArcadeTrick::method, "Method"},
    TrickTableRow{TrickFamily::grab, TrickDirection::up, 2, ArcadeTrick::rocket_air, "Rocket Air"},
    TrickTableRow{TrickFamily::grab, TrickDirection::right, 2, ArcadeTrick::stiffy, "Stiffy"},
    TrickTableRow{TrickFamily::grab, TrickDirection::down, 2, ArcadeTrick::one_foot_tailgrab, "One Foot Tailgrab"},
    TrickTableRow{TrickFamily::grab, TrickDirection::up_left, 2, ArcadeTrick::one_foot_japan, "One Foot Japan"},
    TrickTableRow{TrickFamily::grab, TrickDirection::up_right, 2, ArcadeTrick::judo, "Judo"},
    TrickTableRow{TrickFamily::grab, TrickDirection::down_left, 2, ArcadeTrick::sacktap, "Sacktap"},
    TrickTableRow{TrickFamily::grab, TrickDirection::down_right, 2, ArcadeTrick::christ_air, "Christ Air"},
    TrickTableRow{TrickFamily::grind, TrickDirection::up, 1, ArcadeTrick::nosegrind, "Nosegrind"},
    TrickTableRow{TrickFamily::grind, TrickDirection::down, 1, ArcadeTrick::grind_5_0, "5-0"},
    TrickTableRow{TrickFamily::grind, TrickDirection::left, 1, ArcadeTrick::noseslide_or_tailslide, "Noseslide / Tailslide"},
    TrickTableRow{TrickFamily::grind, TrickDirection::right, 1, ArcadeTrick::noseslide_or_tailslide, "Noseslide / Tailslide"},
    TrickTableRow{TrickFamily::grind, TrickDirection::up_left, 1, ArcadeTrick::crooked_or_overcrook, "Crooked / Overcrook"},
    TrickTableRow{TrickFamily::grind, TrickDirection::up_right, 1, ArcadeTrick::crooked_or_overcrook, "Crooked / Overcrook"},
    TrickTableRow{TrickFamily::grind, TrickDirection::down_left, 1, ArcadeTrick::smith_or_feeble, "Smith / Feeble"},
    TrickTableRow{TrickFamily::grind, TrickDirection::down_right, 1, ArcadeTrick::smith_or_feeble, "Smith / Feeble"},
    TrickTableRow{TrickFamily::grind, TrickDirection::up, 2, ArcadeTrick::nosegrind_to_pivot, "Nosegrind to Pivot"},
    TrickTableRow{TrickFamily::grind, TrickDirection::down, 2, ArcadeTrick::grind_5_0_overturn, "5-0 Overturn"},
    TrickTableRow{TrickFamily::grind, TrickDirection::left, 2, ArcadeTrick::hurricane, "Hurricane"},
    TrickTableRow{TrickFamily::grind, TrickDirection::right, 2, ArcadeTrick::salad, "Salad"},
    TrickTableRow{TrickFamily::grind, TrickDirection::up_left, 2, ArcadeTrick::hang_ten_nosegrind, "Hang Ten Nosegrind"},
    TrickTableRow{TrickFamily::grind, TrickDirection::up_right, 2, ArcadeTrick::crail_slide, "Crail Slide"},
    TrickTableRow{TrickFamily::grind, TrickDirection::down_left, 2, ArcadeTrick::double_blunt_slide, "Double Blunt Slide"},
    TrickTableRow{TrickFamily::grind, TrickDirection::down_right, 2, ArcadeTrick::darkslide, "Darkslide"}
};

constexpr ArcadeTrick select_named_trick(TrickFamily family, TrickDirection direction, unsigned presses) noexcept {
    for (const auto& row : literal_trick_table)
        if (row.family == family && row.direction == direction && row.presses == presses) return row.trick;
    return ArcadeTrick::none; // Never substitute a lower repetition or a different direction.
}
constexpr std::string_view trick_name(ArcadeTrick trick) noexcept {
    for (const auto& row : literal_trick_table)
        if (row.trick == trick) return row.name;
    switch (trick) {
    case ArcadeTrick::grind_50_50: return "50-50";
    case ArcadeTrick::boardslide_or_lipslide: return "Boardslide / Lipslide";
    case ArcadeTrick::nosebluntslide: return "Nosebluntslide";
    case ArcadeTrick::bluntslide: return "Bluntslide";
    default: return {};
    }
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
