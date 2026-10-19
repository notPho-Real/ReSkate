#pragma once
#include "arcade_input_events.h"
#include "arcade_trick_table.h"
#include <cmath>
#include <optional>

namespace dingosdk::arcade_mode {
enum class AirInputPredicate : std::uint8_t { direction, face, repeated_direction, primitive };
// Selection parameters only. Animation-frame gates and initialization percent
// are units for a real native provider, never elapsed-time deadlines.
struct AirSelection {
    float authored_speed{1}, max_speed{1.3f};
    ArcadeTrick trick{};
    TrickFamily family{};
    AirInputPredicate predicate{};
    std::uint8_t trick_slack{10}, grind_slack{25}, minimum_percent{};
    bool extra : 1 {};
    bool use_current : 1 {};
    bool force_init : 1 {};
    bool board_rotate : 1 {};
    constexpr std::uint16_t query_ms() const noexcept {
        return extra || predicate == AirInputPredicate::repeated_direction ? 300 : 500;
    }
    constexpr std::uint16_t release_mask() const noexcept { return family == TrickFamily::grab ? 0x2000 : 0; }
    bool operator==(const AirSelection&) const = default;
    template<class Profile>
    std::optional<float> selected_speed(const Profile& profile) const noexcept {
        if (trick == ArcadeTrick::none || trick == ArcadeTrick::basic_bs_grab ||
            !std::isfinite(authored_speed) || authored_speed <= 0 ||
            (family == TrickFamily::flip && (!std::isfinite(max_speed) || max_speed <= 0))) return {};
        const auto speed = family == TrickFamily::flip ? profile.flip_speed(authored_speed,max_speed) : authored_speed;
        return std::isfinite(speed) && speed > 0 ? std::optional<float>{speed} : std::nullopt;
    }
};
struct AirDirectionRow { std::int16_t code; TrickDirection direction; };
inline constexpr std::array air_direction_rows{
    AirDirectionRow{7,TrickDirection::up_left}, AirDirectionRow{9,TrickDirection::up_right},
    AirDirectionRow{1,TrickDirection::down_left}, AirDirectionRow{3,TrickDirection::down_right},
    AirDirectionRow{8,TrickDirection::up}, AirDirectionRow{2,TrickDirection::down},
    AirDirectionRow{4,TrickDirection::left}, AirDirectionRow{6,TrickDirection::right}};
constexpr TrickDirection air_trick_direction(std::int16_t code) noexcept {
    for (const auto row : air_direction_rows) if (row.code == code) return row.direction;
    return TrickDirection::none;
}
constexpr AirSelection air_selection(TrickFamily family, std::int16_t direction, unsigned presses) noexcept {
    AirSelection result{}; result.family = family;
    if (family != TrickFamily::flip && family != TrickFamily::grab) return result;
    result.trick = select_named_trick(family,air_trick_direction(direction),presses);
    result.extra = presses > 1;
    result.predicate = result.extra ? AirInputPredicate::face : AirInputPredicate::direction;
    if (family == TrickFamily::grab) {
        result.max_speed = 0; result.grind_slack = 0; result.trick_slack = 0; result.minimum_percent = 60;
        if (result.trick == ArcadeTrick::basic_bs_grab) {
            result.minimum_percent = 0; result.predicate = AirInputPredicate::primitive; return result;
        }
        switch (result.trick) {
        case ArcadeTrick::method: result.authored_speed = 1.2f; break;
        case ArcadeTrick::stiffy: result.authored_speed = 1.25f; result.force_init = true; break;
        case ArcadeTrick::one_foot_japan: result.authored_speed = 1.3f; result.minimum_percent = 70; break;
        case ArcadeTrick::madonna: result.minimum_percent = 70; break;
        case ArcadeTrick::sacktap: result.authored_speed = 1.5f; result.trick_slack = 20; result.force_init = true; break;
        case ArcadeTrick::benihana: case ArcadeTrick::christ_air: result.force_init = true; break;
        default: break;
        }
        return result;
    }
    result.use_current = result.extra;
    switch (result.trick) {
    case ArcadeTrick::pop_shove_it: case ArcadeTrick::hardflip:
    case ArcadeTrick::varial_kickflip: case ArcadeTrick::varial_heelflip: result.board_rotate = true; break;
    case ArcadeTrick::inward_heelflip: result.board_rotate = true; result.trick_slack = 15; break;
    case ArcadeTrick::flip_360: result.authored_speed = .9f; break;
    case ArcadeTrick::heelflip_360: result.authored_speed = 1.3f; result.trick_slack = 20; break;
    case ArcadeTrick::hardflip_360: result.authored_speed = 1.1f; result.trick_slack = 20; break;
    case ArcadeTrick::inward_heelflip_360: result.trick_slack = 15; break;
    case ArcadeTrick::double_impossible: case ArcadeTrick::triple_impossible:
        result.authored_speed = 1.2f; result.trick_slack = 15; break;
    case ArcadeTrick::double_kickflip: case ArcadeTrick::double_heelflip: result.trick_slack = 15; break;
    case ArcadeTrick::shove_it_540: result.board_rotate = true; break;
    default: break;
    }
    return result;
}
struct AirDirectionMatch { InputMatch match{}; std::int16_t direction{}; };
inline AirDirectionMatch match_air_direction(const InputEvents& events, TrickFamily family,
        std::uint64_t now, std::uint32_t reject) noexcept {
    const InputToken face{InputSource::buttons,static_cast<std::int16_t>(family == TrickFamily::flip ? 14 : 13),InputEdge::press};
    for (std::size_t i=0;i<air_direction_rows.size();++i) {
        const auto row = air_direction_rows[family == TrickFamily::grab && i>=6 ? 13-i : i];
        const auto definition = air_selection(family,row.code,1);
        if (definition.trick == ArcadeTrick::none) continue;
        const InputToken direction{InputSource::direction,row.code,InputEdge::press};
        const auto match = events.history().air_action(face,direction,events.held(direction),definition.query_ms(),now,reject);
        if (match) return {match,row.code};
    }
    return {};
}
}
