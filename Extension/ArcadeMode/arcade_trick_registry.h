#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include <span>

namespace dingosdk::arcade_mode {
// Physical control ABI and recognizer timing shared by runtime and presentation.
// These are pattern descriptors, not a second per-trick binding table.
struct TrickControls {
    static constexpr std::int16_t ollie=12, grab=13, flip=14, grind=15, grab_layer=7, revert_trigger=1;
    static constexpr std::uint32_t mask(std::int16_t code) noexcept {return 1u<<code;}
    static constexpr std::uint32_t trigger_mask(std::int16_t code) noexcept {return 1u<<(16+code);}
};
struct InputTiming {
    static constexpr std::uint64_t manual_throw_pair_ms=450;
    static constexpr std::uint64_t ground_pair_ms=220, ground_sample_slack_ms=55;
    static constexpr std::uint64_t air_pair_ms=400, air_request_ms=700;
    static constexpr std::uint64_t direction_ms=500, extra_press_ms=300;
    static constexpr std::uint64_t takeoff_preload_ms=160, readiness_ms=200, native_sample_lease_ms=150;
};
// Semantic requests only. A native adapter must separately establish exact support.
enum class TrickDirection : std::uint8_t { none, left, up, right, down, up_left, up_right, down_left, down_right, invalid };
enum class TrickFamily : std::uint8_t { flip, grab, grind };
// Established-rail two-button targets. These retain the existing semantic
// variant numbers; none is a native animation or physics enum value.
enum class GrindChangeTarget : std::uint8_t {
    none, grind_50_50, noseslide, nosegrind, crooked, bluntslide,
    nosebluntslide, smith, grind_5_0, tailslide
};
constexpr std::uint8_t grind_change_variant(GrindChangeTarget target) noexcept {
    return static_cast<std::uint8_t>(target);
}
constexpr GrindChangeTarget grind_change_target(std::uint8_t variant) noexcept {
    return variant >= 1 && variant <= 9 ? static_cast<GrindChangeTarget>(variant) : GrindChangeTarget::none;
}
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
    hurricane, salad, hang_ten_nosegrind, crail_slide, double_blunt_slide, darkslide,
    basic_bs_grab, compact_grab, pressure_turn_flip,
    crossbone, fs_nosebone, fs_tailbone, tuck_knee,
    double_grab, double_grab_front, double_grab_back, double_grab_left, double_grab_right,
    tail_bs_shifty, tail_fs_shifty, tail_nosebone, nose_bs_shifty, nose_fs_shifty, nose_tailbone, nosegrab_alternate, nose_crail_fs_shifty, seatbelt_fs_shifty, seatbelt, crail, crail_fs_shifty, crail_bs_shifty, crail_bs_shifty_alternate, stale_nosebone, stale_tailbone, stale_tuck_knee_right, stale_tuck_knee, stale_tuck_knee_middle, stale, weddle_japan, weddle, weddle_alternate, weddle_nose, weddle_tail,
    fs_one_foot, frigid_air, no_foot, bs_one_foot, tailwalk, dog_piss, nose_judo, superdude,
    seatbelt_bs_shifty, ollie, manual, nose_manual, revert, no_comply, boneless, coffin,
    spin_900, wallplant, special_grab, special_grind, flatground_special,
    finger_1, finger_2, finger_3, finger_4, finger_5, finger_6, finger_7, finger_8, finger_9, finger_10, finger_11, finger_12, finger_13, finger_14, finger_19, finger_20, finger_21, finger_22, finger_23, finger_24, finger_25, finger_26, finger_27, finger_28, finger_29, finger_30, boardslide, lipslide, noseslide, tailslide, crooked, overcrook, smith, feeble
};

}
namespace dingosdk::arcade_mode::native_grab {
struct Control {
    unsigned hands{};
    std::int16_t x{}, y{};
    std::uint16_t feet{}; // Original face-button foot input, only after native grab entry.
    bool transition{}; // Original grab-to-flip input owns neutral RS during catch too.
    bool selector() const noexcept {return x!=0 || y!=0;}
};
// Profiles describe native hand/selector semantics in the original stance.
// After a leading-foot change, the game's producer swaps the trigger hands
// and horizontal selector. Vertical selectors and anatomical foot buttons
// retain their existing convention. Authored fingerflip X is already mirrored
// by its own playback and must not be mirrored a second time here.
constexpr Control physical_control(Control control,bool mirror_x) noexcept {
    if (!mirror_x) {
        control.hands=((control.hands&1u)<<1)|((control.hands&2u)>>1);
        if (!control.transition) control.x=static_cast<std::int16_t>(-control.x);
    }
    return control;
}
struct Pilot {
    ArcadeTrick trick; std::int32_t detail; float x, y; std::int32_t family{2}; unsigned hands{2};
    std::int16_t selector_x{}, selector_y{};
    std::int32_t one_footer{};
    unsigned foot{}; // 1 left, 2 right, 3 both; not an animation identity.
    unsigned actor_index{}; // Original contract::actor_assets slot, independent of catalog order.
    constexpr Control control() const noexcept {return {hands,selector_x,selector_y};}
};

}
namespace dingosdk::arcade_mode {
enum class TrickCategory : std::uint8_t { flips, grabs, grinds, manuals, footplants, walls, flatground, specials };
enum class TrickStatus : std::uint8_t { verified, experimental, planned };
enum class TrickExecutor : std::uint8_t { none, native_flip, native_grab, held_flip, native_ollie, native_manual, native_revert, native_footplant, native_ground_grab, native_rail, native_fingerflip };
enum class BindingPattern : std::uint8_t { named, extended_grab, selected_grab, neutral_flip, held_repeat, ollie_release, manual_pair, footplant, ground_grab, revert, rail_entry, fingerflip, rail_change, sequence };
enum class BindingContext : std::uint8_t { air, ground, ground_or_air, rail_approach, established_rail };
inline constexpr unsigned footplant_single_ms=300, footplant_repeat_ms=500;
inline constexpr unsigned grab_repeat_ms=250, flip_repeat_ms=233;
enum class StepKind : std::uint8_t { press, hold, release, direction, release_direction, then, together };
struct InputStep {
    StepKind kind{};
    std::uint32_t buttons{};
    TrickDirection direction{};
    bool operator==(const InputStep&) const = default;
};
struct BindingSequence {
    std::array<InputStep,16> steps{};
    unsigned count{}, window_ms{};
    bool operator==(const BindingSequence&) const = default;
};
struct TrickBinding {
    BindingPattern pattern{};
    TrickDirection direction{};
    unsigned presses{1};
    ArcadeTrick variant{};
    BindingContext context{BindingContext::air};
    unsigned parameter{};
    std::int16_t first_button{}, second_button{};
    BindingSequence sequence{};
    bool operator==(const TrickBinding&) const = default;
};
enum class Acceptance : unsigned { action=1, physics=2, recognition=4, exit_landing=8, scoring_combo=16 };
enum class EvidenceKind : std::uint8_t { none, native_gameplay, source_asset, automated_test };
struct NativeEvidence {
    Acceptance criterion{};
    std::string_view capture, observation;
    bool invalidated{};
    EvidenceKind kind{};
    unsigned repeat_count{};
};
struct TrickEvidence {
    std::string_view revision, context, reference, remaining;
    unsigned game_build{20260929}, required{31};
    bool invalidated{};
    std::array<NativeEvidence,5> observations{};
    std::string_view implementation_fingerprint{};
};
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
        ContextualPredicate::press,static_cast<std::uint16_t>(InputTiming::extra_press_ms),TrickControls::grab,0,contextual_extra},
    ContextualTrickRow{InputAirTableKind::pressure,ArcadeTrick::pressure_turn_flip,TrickFamily::flip,
        ContextualPredicate::air_direction,static_cast<std::uint16_t>(InputTiming::extra_press_ms),TrickControls::flip,6,contextual_board_rotation|contextual_followup_flip}};
constexpr const ContextualTrickRow* contextual_trick(InputAirTableKind table) noexcept {
    for (const auto& row : contextual_trick_table) if (row.table == table) return &row;
    return nullptr;
}
struct SpecialMetadata {
    ArcadeTrick preceding_trick{};
    std::string_view required_state, native_animation, combo_requirement;
};
// Legacy geometry requests are selector identities, not independently executable tricks.
// Keep their original enum ABI while giving each possible outcome a canonical record.
struct RailSelector {
    ArcadeTrick request;
    std::array<TrickDirection,2> directions;
    std::array<ArcadeTrick,2> outcomes;
};
inline constexpr std::array rail_selectors{
    RailSelector{ArcadeTrick::boardslide_or_lipslide,{TrickDirection::none,TrickDirection::invalid},{ArcadeTrick::boardslide,ArcadeTrick::lipslide}},
    RailSelector{ArcadeTrick::noseslide_or_tailslide,{TrickDirection::left,TrickDirection::right},{ArcadeTrick::noseslide,ArcadeTrick::tailslide}},
    RailSelector{ArcadeTrick::crooked_or_overcrook,{TrickDirection::up_left,TrickDirection::up_right},{ArcadeTrick::crooked,ArcadeTrick::overcrook}},
    RailSelector{ArcadeTrick::smith_or_feeble,{TrickDirection::down_left,TrickDirection::down_right},{ArcadeTrick::smith,ArcadeTrick::feeble}}};
struct FingerflipBinding { int identity{}, family{}; unsigned gestures{}; int destination{}; unsigned actor_index{}; };
struct TrickDefinition {
    ArcadeTrick id;
    std::string_view key, name;
    TrickCategory category;
    TrickExecutor executor;
    std::array<TrickBinding,8> bindings{};
    unsigned binding_count{};
    std::uint8_t native_flip{}, nollie_flip{}, held_base{};
    std::array<native_grab::Pilot,3> grab_variants{};
    unsigned grab_variant_count{};
    TrickEvidence evidence{};
    FingerflipBinding fingerflip{};
    SpecialMetadata special{};
};
#include "arcade_trick_evidence.inl"
inline constexpr auto trick_registry=[] {
std::array entries{
    TrickDefinition{ArcadeTrick::kickflip,"kickflip","Kickflip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::left,1,ArcadeTrick::kickflip,BindingContext::air,0}, {BindingPattern::neutral_flip,TrickDirection::none,1,ArcadeTrick::kickflip,BindingContext::air,0}}},2,2,17,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::impossible,"impossible","Impossible",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::up,1,ArcadeTrick::impossible,BindingContext::air,0}}},1,36,38,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::heelflip,"heelflip","Heelflip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::right,1,ArcadeTrick::heelflip,BindingContext::air,0}}},1,3,18,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::pop_shove_it,"pop_shove_it","Pop Shove-It",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::down,1,ArcadeTrick::pop_shove_it,BindingContext::air,0}}},1,4,19,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::hardflip,"hardflip","Hardflip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::up_left,1,ArcadeTrick::hardflip,BindingContext::air,0}}},1,9,20,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::inward_heelflip,"inward_heelflip","Inward Heelflip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::up_right,1,ArcadeTrick::inward_heelflip,BindingContext::air,0}}},1,6,24,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::varial_kickflip,"varial_kickflip","Varial Kickflip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::down_left,1,ArcadeTrick::varial_kickflip,BindingContext::air,0}}},1,5,23,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::varial_heelflip,"varial_heelflip","Varial Heelflip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::down_right,1,ArcadeTrick::varial_heelflip,BindingContext::air,0}}},1,8,21,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::double_kickflip,"double_kickflip","Double Kickflip",TrickCategory::flips,TrickExecutor::held_flip,
        {{{BindingPattern::named,TrickDirection::left,2,ArcadeTrick::double_kickflip,BindingContext::air,0}, {BindingPattern::held_repeat,TrickDirection::none,2,ArcadeTrick::double_kickflip,BindingContext::air,0}}},2,0,0,2,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::double_impossible,"double_impossible","Double Impossible",TrickCategory::flips,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::up,2,ArcadeTrick::double_impossible,BindingContext::air,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::double_heelflip,"double_heelflip","Double Heelflip",TrickCategory::flips,TrickExecutor::held_flip,
        {{{BindingPattern::named,TrickDirection::right,2,ArcadeTrick::double_heelflip,BindingContext::air,0}}},1,0,0,3,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::shove_it_360,"shove_it_360","360 Shove-It",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::down,2,ArcadeTrick::shove_it_360,BindingContext::air,0}}},1,10,28,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::hardflip_360,"hardflip_360","360 Hardflip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::up_left,2,ArcadeTrick::hardflip_360,BindingContext::air,0}}},1,15,26,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::inward_heelflip_360,"inward_heelflip_360","360 Inward Heelflip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::up_right,2,ArcadeTrick::inward_heelflip_360,BindingContext::air,0}}},1,12,30,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::flip_360,"flip_360","360 Flip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::down_left,2,ArcadeTrick::flip_360,BindingContext::air,0}}},1,11,29,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::heelflip_360,"heelflip_360","360 Heelflip",TrickCategory::flips,TrickExecutor::native_flip,
        {{{BindingPattern::named,TrickDirection::down_right,2,ArcadeTrick::heelflip_360,BindingContext::air,0}}},1,14,27,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::triple_kickflip,"triple_kickflip","Triple Kickflip",TrickCategory::flips,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::left,3,ArcadeTrick::triple_kickflip,BindingContext::air,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::triple_impossible,"triple_impossible","Triple Impossible",TrickCategory::flips,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::up,3,ArcadeTrick::triple_impossible,BindingContext::air,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::triple_heelflip,"triple_heelflip","Triple Heelflip",TrickCategory::flips,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::right,3,ArcadeTrick::triple_heelflip,BindingContext::air,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::shove_it_540,"shove_it_540","540 Shove-It",TrickCategory::flips,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::down,3,ArcadeTrick::shove_it_540,BindingContext::air,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::melon,"melon","Melon",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::left,1,ArcadeTrick::melon,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::melon,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::melon,4,0,1,2,2,0,0,0,0,1}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::nosegrab,"nosegrab","Nosegrab",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::up,1,ArcadeTrick::nosegrab,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::nosegrab,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::nosegrab_alternate,BindingContext::air,0}}},3,0,0,0,{{{ArcadeTrick::nosegrab,21,0,0,6,2,0,-28000,0,0,21}, {ArcadeTrick::nosegrab_alternate,21,0,-0.49f,6,2,0,-28000,0,0,23}}},2,rc3_evidence},
    TrickDefinition{ArcadeTrick::indy,"indy","FS Grab",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::right,1,ArcadeTrick::indy,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::indy,BindingContext::air,0}, {BindingPattern::ground_grab,TrickDirection::right,1,ArcadeTrick::indy,BindingContext::ground,1}}},3,0,0,0,{{{ArcadeTrick::indy,6,0,0,1,1,0,0,0,0,5}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::tailgrab,"tailgrab","Tailgrab",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::down,1,ArcadeTrick::tailgrab,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::tailgrab,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::tailgrab,11,0,0,3,1,0,28000,0,0,17}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::japan,"japan","Japan",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::up_left,1,ArcadeTrick::japan,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::japan,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::japan,3,0,-1,2,2,0,0,0,0,3}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::madonna,"madonna","Madonna",TrickCategory::grabs,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::up_right,1,ArcadeTrick::madonna,BindingContext::air,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::benihana,"benihana","Benihana",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::down_left,1,ArcadeTrick::benihana,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::benihana,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::benihana,0,0,0,3,1,0,28000,7,1,49}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::airwalk,"airwalk","Airwalk",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::down_right,1,ArcadeTrick::airwalk,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::airwalk,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::airwalk,0,0,0,6,2,0,-28000,12,3,54}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::method,"method","Method",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::left,2,ArcadeTrick::method,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::method,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::method,2,1,0,2,2,0,0,0,0,2}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::rocket_air,"rocket_air","Rocket Air",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::up,2,ArcadeTrick::rocket_air,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::rocket_air,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::rocket_air,38,0,0,10,3,0,-28000,0,0,42}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::stiffy,"stiffy","Stiffy",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::right,2,ArcadeTrick::stiffy,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::stiffy,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::stiffy,10,-1,0,1,1,0,0,0,0,9}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::one_foot_tailgrab,"one_foot_tailgrab","One Foot Tailgrab",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::down,2,ArcadeTrick::one_foot_tailgrab,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::one_foot_tailgrab,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::one_foot_tailgrab,0,0,0,3,1,0,28000,8,2,50}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::one_foot_japan,"one_foot_japan","One Foot Japan",TrickCategory::grabs,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::up_left,2,ArcadeTrick::one_foot_japan,BindingContext::air,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::judo,"judo","Judo",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::up_right,2,ArcadeTrick::judo,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::judo,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::judo,0,0,0,2,2,0,0,4,1,46}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::sacktap,"sacktap","Sacktap",TrickCategory::grabs,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::down_left,2,ArcadeTrick::sacktap,BindingContext::air,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::christ_air,"christ_air","Christ Air",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::down_right,2,ArcadeTrick::christ_air,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::christ_air,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::christ_air,0,0,0,2,2,0,0,6,3,48}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::grind_50_50,"grind_50_50","50-50",TrickCategory::grinds,TrickExecutor::native_rail,
        {{{BindingPattern::rail_entry,TrickDirection::none,1,ArcadeTrick::grind_50_50,BindingContext::rail_approach,1}, {BindingPattern::rail_change,TrickDirection::none,2,ArcadeTrick::grind_50_50,BindingContext::established_rail,1,15,15}}},2,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::boardslide,"boardslide","Boardslide",TrickCategory::grinds,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::lipslide,"lipslide","Lipslide",TrickCategory::grinds,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::nosegrind,"nosegrind","Nosegrind",TrickCategory::grinds,TrickExecutor::native_rail,
        {{{BindingPattern::named,TrickDirection::up,1,ArcadeTrick::nosegrind,BindingContext::rail_approach,0}, {BindingPattern::rail_change,TrickDirection::none,2,ArcadeTrick::nosegrind,BindingContext::established_rail,3,15,13}}},2,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::grind_5_0,"grind_5_0","5-0",TrickCategory::grinds,TrickExecutor::native_rail,
        {{{BindingPattern::named,TrickDirection::down,1,ArcadeTrick::grind_5_0,BindingContext::rail_approach,0}, {BindingPattern::rail_change,TrickDirection::none,2,ArcadeTrick::grind_5_0,BindingContext::established_rail,8,14,13}}},2,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::noseslide,"noseslide","Noseslide",TrickCategory::grinds,TrickExecutor::native_rail,
        {{{BindingPattern::rail_change,TrickDirection::none,2,ArcadeTrick::noseslide,BindingContext::established_rail,2,15,14}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::tailslide,"tailslide","Tailslide",TrickCategory::grinds,TrickExecutor::native_rail,
        {{{BindingPattern::rail_change,TrickDirection::none,2,ArcadeTrick::tailslide,BindingContext::established_rail,9,14,15}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::crooked,"crooked","Crooked",TrickCategory::grinds,TrickExecutor::native_rail,
        {{{BindingPattern::rail_change,TrickDirection::none,2,ArcadeTrick::crooked,BindingContext::established_rail,4,13,13}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::overcrook,"overcrook","Overcrook",TrickCategory::grinds,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::smith,"smith","Smith",TrickCategory::grinds,TrickExecutor::native_rail,
        {{{BindingPattern::rail_change,TrickDirection::none,2,ArcadeTrick::smith,BindingContext::established_rail,7,14,14}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::feeble,"feeble","Feeble",TrickCategory::grinds,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::nosebluntslide,"nosebluntslide","Nosebluntslide",TrickCategory::grinds,TrickExecutor::native_rail,
        {{{BindingPattern::rail_entry,TrickDirection::up,1,ArcadeTrick::nosebluntslide,BindingContext::rail_approach,2}, {BindingPattern::rail_change,TrickDirection::none,2,ArcadeTrick::nosebluntslide,BindingContext::established_rail,6,13,15}}},2,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::bluntslide,"bluntslide","Bluntslide",TrickCategory::grinds,TrickExecutor::native_rail,
        {{{BindingPattern::rail_entry,TrickDirection::down,1,ArcadeTrick::bluntslide,BindingContext::rail_approach,2}, {BindingPattern::rail_change,TrickDirection::none,2,ArcadeTrick::bluntslide,BindingContext::established_rail,5,13,14}}},2,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::nosegrind_to_pivot,"nosegrind_to_pivot","Nosegrind to Pivot",TrickCategory::grinds,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::up,2,ArcadeTrick::nosegrind_to_pivot,BindingContext::rail_approach,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::grind_5_0_overturn,"grind_5_0_overturn","5-0 Overturn",TrickCategory::grinds,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::down,2,ArcadeTrick::grind_5_0_overturn,BindingContext::rail_approach,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::hurricane,"hurricane","Hurricane",TrickCategory::grinds,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::left,2,ArcadeTrick::hurricane,BindingContext::rail_approach,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::salad,"salad","Salad",TrickCategory::grinds,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::right,2,ArcadeTrick::salad,BindingContext::rail_approach,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::hang_ten_nosegrind,"hang_ten_nosegrind","Hang Ten Nosegrind",TrickCategory::grinds,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::up_left,2,ArcadeTrick::hang_ten_nosegrind,BindingContext::rail_approach,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::crail_slide,"crail_slide","Crail Slide",TrickCategory::grinds,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::up_right,2,ArcadeTrick::crail_slide,BindingContext::rail_approach,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::double_blunt_slide,"double_blunt_slide","Double Blunt Slide",TrickCategory::grinds,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::down_left,2,ArcadeTrick::double_blunt_slide,BindingContext::rail_approach,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::darkslide,"darkslide","Darkslide",TrickCategory::grinds,TrickExecutor::none,
        {{{BindingPattern::named,TrickDirection::down_right,2,ArcadeTrick::darkslide,BindingContext::rail_approach,0}}},1,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::basic_bs_grab,"basic_bs_grab","BS Grab",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::named,TrickDirection::none,1,ArcadeTrick::basic_bs_grab,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::basic_bs_grab,BindingContext::air,0}, {BindingPattern::ground_grab,TrickDirection::left,1,ArcadeTrick::basic_bs_grab,BindingContext::ground,2}}},3,0,0,0,{{{ArcadeTrick::basic_bs_grab,1,0,0,2,2,0,0,0,0,0}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::compact_grab,"compact_grab","Compact Grab",TrickCategory::specials,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::pressure_turn_flip,"pressure_turn_flip","Pressure Turn Flip",TrickCategory::specials,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::crossbone,"crossbone","Crossbone",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::crossbone,BindingContext::air,0}, {BindingPattern::extended_grab,TrickDirection::up_left,1,ArcadeTrick::crossbone,BindingContext::air,7}}},2,0,0,0,{{{ArcadeTrick::crossbone,5,-1,0,2,2,0,0,0,0,4}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::fs_nosebone,"fs_nosebone","FS Nosebone",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::fs_nosebone,BindingContext::air,0}, {BindingPattern::extended_grab,TrickDirection::up_right,1,ArcadeTrick::fs_nosebone,BindingContext::air,9}}},2,0,0,0,{{{ArcadeTrick::fs_nosebone,7,0,1,1,1,0,0,0,0,6}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::fs_tailbone,"fs_tailbone","FS Tailbone",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::fs_tailbone,BindingContext::air,0}, {BindingPattern::extended_grab,TrickDirection::down_left,1,ArcadeTrick::fs_tailbone,BindingContext::air,1}}},2,0,0,0,{{{ArcadeTrick::fs_tailbone,8,0,-1,1,1,0,0,0,0,7}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::tuck_knee,"tuck_knee","Tuck Knee",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::tuck_knee,BindingContext::air,0}, {BindingPattern::extended_grab,TrickDirection::down_right,1,ArcadeTrick::tuck_knee,BindingContext::air,3}}},2,0,0,0,{{{ArcadeTrick::tuck_knee,9,1,0,1,1,0,0,0,0,8}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::double_grab,"double_grab","Double Grab",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::double_grab,BindingContext::air,0}, {BindingPattern::ground_grab,TrickDirection::none,1,ArcadeTrick::double_grab,BindingContext::ground,3}}},2,0,0,0,{{{ArcadeTrick::double_grab,33,0,0,9,3,0,0,0,0,10}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::double_grab_front,"double_grab_front","Double Grab Front",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::double_grab_front,BindingContext::air,0}, {BindingPattern::extended_grab,TrickDirection::up,1,ArcadeTrick::double_grab_front,BindingContext::air,8}}},2,0,0,0,{{{ArcadeTrick::double_grab_front,36,0,1,9,3,0,0,0,0,11}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::double_grab_back,"double_grab_back","Double Grab Back",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::double_grab_back,BindingContext::air,0}, {BindingPattern::extended_grab,TrickDirection::down,1,ArcadeTrick::double_grab_back,BindingContext::air,2}}},2,0,0,0,{{{ArcadeTrick::double_grab_back,37,0,-1,9,3,0,0,0,0,12}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::double_grab_left,"double_grab_left","Double Grab Left",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::double_grab_left,BindingContext::air,0}, {BindingPattern::extended_grab,TrickDirection::left,1,ArcadeTrick::double_grab_left,BindingContext::air,4}}},2,0,0,0,{{{ArcadeTrick::double_grab_left,34,-1,0,9,3,0,0,0,0,13}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::double_grab_right,"double_grab_right","Double Grab Right",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::double_grab_right,BindingContext::air,0}, {BindingPattern::extended_grab,TrickDirection::right,1,ArcadeTrick::double_grab_right,BindingContext::air,6}}},2,0,0,0,{{{ArcadeTrick::double_grab_right,35,1,0,9,3,0,0,0,0,14}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::tail_bs_shifty,"tail_bs_shifty","Tail BS Shifty",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::tail_bs_shifty,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::tail_bs_shifty,14,-1,-0.01f,3,1,0,28000,0,0,15}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::tail_fs_shifty,"tail_fs_shifty","Tail FS Shifty",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::tail_fs_shifty,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::tail_fs_shifty,13,1,0,3,1,0,28000,0,0,16}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::tail_nosebone,"tail_nosebone","Tail NoseBone",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::tail_nosebone,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::tail_nosebone,12,0,1,3,1,0,28000,0,0,18}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::nose_bs_shifty,"nose_bs_shifty","Nose BS Shifty",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::nose_bs_shifty,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::nose_bs_shifty,24,-1,0,6,2,0,-28000,0,0,19}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::nose_fs_shifty,"nose_fs_shifty","Nose FS Shifty",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::nose_fs_shifty,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::nose_fs_shifty,23,1,0,6,2,0,-28000,0,0,20}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::nose_tailbone,"nose_tailbone","Nose Tailbone",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::nose_tailbone,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::nose_tailbone,22,0,-1,6,2,0,-28000,0,0,22}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::seatbelt_fs_shifty,"seatbelt_fs_shifty","Seatbelt FS Shifty",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::seatbelt_fs_shifty,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::seatbelt_fs_shifty,16,-1,0,4,2,0,28000,0,0,25}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::seatbelt,"seatbelt","Seatbelt",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::seatbelt,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::seatbelt,15,0,0,4,2,0,28000,0,0,26}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::crail,"crail","Crail",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::crail,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::crail,18,0,0,5,1,0,-28000,0,0,27}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::crail_fs_shifty,"crail_fs_shifty","Crail FS Shifty",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::crail_fs_shifty,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::nose_crail_fs_shifty,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::crail_fs_shifty,19,1,0,5,1,0,-28000,0,0,28}, {ArcadeTrick::nose_crail_fs_shifty,19,0.5f,0,6,2,0,-28000,0,0,24}}},2,rc3_evidence},
    TrickDefinition{ArcadeTrick::crail_bs_shifty,"crail_bs_shifty","Crail BS Shifty",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::crail_bs_shifty,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::crail_bs_shifty_alternate,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::crail_bs_shifty,20,-1,0,5,1,0,-28000,0,0,29}, {ArcadeTrick::crail_bs_shifty_alternate,20,-0.5f,0,5,1,0,-28000,0,0,30}}},2,rc3_evidence},
    TrickDefinition{ArcadeTrick::stale_nosebone,"stale_nosebone","Stale Nosebone",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::stale_nosebone,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::stale_nosebone,27,0.01f,1,7,1,28000,0,0,0,31}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::stale_tailbone,"stale_tailbone","Stale Tailbone",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::stale_tailbone,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::stale_tailbone,26,0,-1,7,1,28000,0,0,0,32}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::stale_tuck_knee,"stale_tuck_knee","Stale TuckKnee",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::stale_tuck_knee,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::stale_tuck_knee_right,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::stale_tuck_knee_middle,BindingContext::air,0}}},3,0,0,0,{{{ArcadeTrick::stale_tuck_knee,28,-1,0,7,1,28000,0,0,0,34}, {ArcadeTrick::stale_tuck_knee_right,28,1,0,7,1,28000,0,0,0,33}, {ArcadeTrick::stale_tuck_knee_middle,28,-0.51f,0,7,1,28000,0,0,0,35}}},3,rc3_evidence},
    TrickDefinition{ArcadeTrick::stale,"stale","Stale",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::stale,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::stale,25,0,0,7,1,28000,0,0,0,36}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::weddle_japan,"weddle_japan","Weddle Japan",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::weddle_japan,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::weddle_japan,32,-1,0,8,2,28000,0,0,0,37}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::weddle,"weddle","Weddle",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::weddle,BindingContext::air,0}, {BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::weddle_alternate,BindingContext::air,0}}},2,0,0,0,{{{ArcadeTrick::weddle,29,0,0,8,2,28000,0,0,0,38}, {ArcadeTrick::weddle_alternate,29,1,0,8,2,28000,0,0,0,39}}},2,rc3_evidence},
    TrickDefinition{ArcadeTrick::weddle_nose,"weddle_nose","Weddle Nose",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::weddle_nose,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::weddle_nose,31,0,1,8,2,28000,0,0,0,40}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::weddle_tail,"weddle_tail","Weddle Tail",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::weddle_tail,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::weddle_tail,30,0,-1,8,2,28000,0,0,0,41}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::fs_one_foot,"fs_one_foot","FS One Foot",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::fs_one_foot,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::fs_one_foot,0,0,0,1,1,0,0,1,1,43}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::frigid_air,"frigid_air","Frigid Air",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::frigid_air,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::frigid_air,0,0,0,1,1,0,0,2,2,44}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::no_foot,"no_foot","No Foot",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::no_foot,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::no_foot,0,0,0,1,1,0,0,3,3,45}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::bs_one_foot,"bs_one_foot","BS One Foot",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::bs_one_foot,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::bs_one_foot,0,0,0,2,2,0,0,5,2,47}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::tailwalk,"tailwalk","Tailwalk",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::tailwalk,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::tailwalk,0,0,0,3,1,0,28000,9,3,51}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::dog_piss,"dog_piss","Dog Piss",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::dog_piss,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::dog_piss,0,0,0,6,2,0,-28000,10,1,52}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::nose_judo,"nose_judo","Nose Judo",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::nose_judo,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::nose_judo,0,0,0,6,2,0,-28000,11,2,53}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::superdude,"superdude","Superdude",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::superdude,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::superdude,0,0,0,9,3,0,0,13,3,55}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::seatbelt_bs_shifty,"seatbelt_bs_shifty","Seatbelt BS Shifty",TrickCategory::grabs,TrickExecutor::native_grab,
        {{{BindingPattern::selected_grab,TrickDirection::none,1,ArcadeTrick::seatbelt_bs_shifty,BindingContext::air,0}}},1,0,0,0,{{{ArcadeTrick::seatbelt_bs_shifty,24,1,0,4,2,0,28000,0,0,56}}},1,rc3_evidence},
    TrickDefinition{ArcadeTrick::ollie,"ollie","Ollie",TrickCategory::flatground,TrickExecutor::native_ollie,
        {{{BindingPattern::ollie_release,TrickDirection::none,1,ArcadeTrick::ollie,BindingContext::ground,0}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::manual,"manual","Manual",TrickCategory::manuals,TrickExecutor::native_manual,
        {{{BindingPattern::manual_pair,TrickDirection::up,1,ArcadeTrick::manual,BindingContext::ground_or_air,0}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::nose_manual,"nose_manual","Nose Manual",TrickCategory::manuals,TrickExecutor::native_manual,
        {{{BindingPattern::manual_pair,TrickDirection::down,1,ArcadeTrick::nose_manual,BindingContext::ground_or_air,0}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::revert,"revert","Revert",TrickCategory::manuals,TrickExecutor::native_revert,
        {{{BindingPattern::revert,TrickDirection::none,1,ArcadeTrick::revert,BindingContext::ground,0}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::no_comply,"no_comply","No Comply",TrickCategory::footplants,TrickExecutor::native_footplant,
        {{{BindingPattern::footplant,TrickDirection::up,1,ArcadeTrick::no_comply,BindingContext::ground,1}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::boneless,"boneless","Boneless",TrickCategory::footplants,TrickExecutor::native_footplant,
        {{{BindingPattern::footplant,TrickDirection::up,2,ArcadeTrick::boneless,BindingContext::ground,2}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::coffin,"coffin","Coffin",TrickCategory::flatground,TrickExecutor::native_ground_grab,
        {{{BindingPattern::ground_grab,TrickDirection::down,1,ArcadeTrick::coffin,BindingContext::ground,4}}},1,0,0,0,{{}},0,rc3_evidence},
    TrickDefinition{ArcadeTrick::spin_900,"spin_900","The 900",TrickCategory::specials,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::wallplant,"wallplant","Wallplant",TrickCategory::walls,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::special_grab,"special_grab","Special Grab",TrickCategory::specials,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::special_grind,"special_grind","Special Grind",TrickCategory::specials,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::flatground_special,"flatground_special","Advanced Flatground",TrickCategory::flatground,TrickExecutor::none,
        {{}},0,0,0,0,{{}},0,planned_evidence},
    TrickDefinition{ArcadeTrick::finger_1,"finger_1","Fingerflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::none,1,ArcadeTrick::finger_1,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::left,1,ArcadeTrick::finger_1,BindingContext::air,2}}},2,0,0,0,{},0,rc3_evidence,{1,6,2,6,0}},
    TrickDefinition{ArcadeTrick::finger_2,"finger_2","Weddleflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::none,1,ArcadeTrick::finger_2,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::left,1,ArcadeTrick::finger_2,BindingContext::air,2}}},2,0,0,0,{},0,rc3_evidence,{2,8,2,8,1}},
    TrickDefinition{ArcadeTrick::finger_3,"finger_3","Weddle To BS",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::down,1,ArcadeTrick::finger_3,BindingContext::air,4},{BindingPattern::fingerflip,TrickDirection::up,1,ArcadeTrick::finger_3,BindingContext::air,7}}},2,0,0,0,{},0,rc3_evidence,{3,8,56,2,2}},
    TrickDefinition{ArcadeTrick::finger_4,"finger_4","Seatbelt To Nose",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::none,1,ArcadeTrick::finger_4,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::left,1,ArcadeTrick::finger_4,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::right,1,ArcadeTrick::finger_4,BindingContext::air,3},{BindingPattern::fingerflip,TrickDirection::down,1,ArcadeTrick::finger_4,BindingContext::air,4},{BindingPattern::fingerflip,TrickDirection::up,1,ArcadeTrick::finger_4,BindingContext::air,7}}},5,0,0,0,{},0,rc3_evidence,{4,4,62,6,3}},
    TrickDefinition{ArcadeTrick::finger_5,"finger_5","Stale To FS",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::down,1,ArcadeTrick::finger_5,BindingContext::air,4},{BindingPattern::fingerflip,TrickDirection::up,1,ArcadeTrick::finger_5,BindingContext::air,7}}},2,0,0,0,{},0,rc3_evidence,{5,7,56,1,4}},
    TrickDefinition{ArcadeTrick::finger_6,"finger_6","Stale Flip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::none,1,ArcadeTrick::finger_6,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::left,1,ArcadeTrick::finger_6,BindingContext::air,2}}},2,0,0,0,{},0,rc3_evidence,{6,7,2,7,5}},
    TrickDefinition{ArcadeTrick::finger_7,"finger_7","Tail Flip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::none,1,ArcadeTrick::finger_7,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::left,1,ArcadeTrick::finger_7,BindingContext::air,2}}},2,0,0,0,{},0,rc3_evidence,{7,3,2,3,6}},
    TrickDefinition{ArcadeTrick::finger_8,"finger_8","Crail To Tail",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::none,1,ArcadeTrick::finger_8,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::left,1,ArcadeTrick::finger_8,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::right,1,ArcadeTrick::finger_8,BindingContext::air,3},{BindingPattern::fingerflip,TrickDirection::down,1,ArcadeTrick::finger_8,BindingContext::air,4},{BindingPattern::fingerflip,TrickDirection::up,1,ArcadeTrick::finger_8,BindingContext::air,7}}},5,0,0,0,{},0,rc3_evidence,{8,5,62,3,7}},
    TrickDefinition{ArcadeTrick::finger_9,"finger_9","BS To Weddle",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::up,1,ArcadeTrick::finger_9,BindingContext::air,7}}},1,0,0,0,{},0,rc3_evidence,{9,2,16,8,8}},
    TrickDefinition{ArcadeTrick::finger_10,"finger_10","BS Varial",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::down,1,ArcadeTrick::finger_10,BindingContext::air,4}}},1,0,0,0,{},0,rc3_evidence,{10,2,40,2,9}},
    TrickDefinition{ArcadeTrick::finger_11,"finger_11","BS Fingerflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::none,1,ArcadeTrick::finger_11,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::left,1,ArcadeTrick::finger_11,BindingContext::air,2}}},2,0,0,0,{},0,rc3_evidence,{11,2,2,2,10}},
    TrickDefinition{ArcadeTrick::finger_12,"finger_12","FS To Stale",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::up,1,ArcadeTrick::finger_12,BindingContext::air,7}}},1,0,0,0,{},0,rc3_evidence,{12,1,16,7,11}},
    TrickDefinition{ArcadeTrick::finger_13,"finger_13","FS Varial",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::down,1,ArcadeTrick::finger_13,BindingContext::air,4}}},1,0,0,0,{},0,rc3_evidence,{13,1,40,1,12}},
    TrickDefinition{ArcadeTrick::finger_14,"finger_14","FS Fingerflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::none,1,ArcadeTrick::finger_14,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::left,1,ArcadeTrick::finger_14,BindingContext::air,2}}},2,0,0,0,{},0,rc3_evidence,{14,1,2,1,13}},
    TrickDefinition{ArcadeTrick::finger_19,"finger_19","Double Grab Kickflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::none,1,ArcadeTrick::finger_19,BindingContext::air,2},{BindingPattern::fingerflip,TrickDirection::left,1,ArcadeTrick::finger_19,BindingContext::air,2}}},2,0,0,0,{},0,rc3_evidence,{19,9,2,9,14}},
    TrickDefinition{ArcadeTrick::finger_20,"finger_20","Double Grab Heelflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::right,1,ArcadeTrick::finger_20,BindingContext::air,3}}},1,0,0,0,{},0,rc3_evidence,{20,9,4,9,15}},
    TrickDefinition{ArcadeTrick::finger_21,"finger_21","Double Grab FS Shuv",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::up,1,ArcadeTrick::finger_21,BindingContext::air,7}}},1,0,0,0,{},0,rc3_evidence,{21,9,16,9,16}},
    TrickDefinition{ArcadeTrick::finger_22,"finger_22","Double Grab BS Shuv",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::down,1,ArcadeTrick::finger_22,BindingContext::air,4}}},1,0,0,0,{},0,rc3_evidence,{22,9,40,9,17}},
    TrickDefinition{ArcadeTrick::finger_23,"finger_23","BS Finger Heelflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::right,1,ArcadeTrick::finger_23,BindingContext::air,3}}},1,0,0,0,{},0,rc3_evidence,{23,2,4,2,18}},
    TrickDefinition{ArcadeTrick::finger_24,"finger_24","FS Finger Heelflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::right,1,ArcadeTrick::finger_24,BindingContext::air,3}}},1,0,0,0,{},0,rc3_evidence,{24,1,4,1,19}},
    TrickDefinition{ArcadeTrick::finger_25,"finger_25","Finger Heelflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::right,1,ArcadeTrick::finger_25,BindingContext::air,3}}},1,0,0,0,{},0,rc3_evidence,{25,6,4,6,20}},
    TrickDefinition{ArcadeTrick::finger_26,"finger_26","Tail Heelflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::right,1,ArcadeTrick::finger_26,BindingContext::air,3}}},1,0,0,0,{},0,rc3_evidence,{26,3,4,3,21}},
    TrickDefinition{ArcadeTrick::finger_27,"finger_27","Weddle Heelflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::right,1,ArcadeTrick::finger_27,BindingContext::air,3}}},1,0,0,0,{},0,rc3_evidence,{27,8,4,8,22}},
    TrickDefinition{ArcadeTrick::finger_28,"finger_28","Stale Heelflip",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::right,1,ArcadeTrick::finger_28,BindingContext::air,3}}},1,0,0,0,{},0,rc3_evidence,{28,7,4,7,23}},
    TrickDefinition{ArcadeTrick::finger_29,"finger_29","Tail To Crail",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::down,1,ArcadeTrick::finger_29,BindingContext::air,4},{BindingPattern::fingerflip,TrickDirection::up,1,ArcadeTrick::finger_29,BindingContext::air,7}}},2,0,0,0,{},0,rc3_evidence,{29,3,56,5,24}},
    TrickDefinition{ArcadeTrick::finger_30,"finger_30","Nose To Seatbelt",TrickCategory::grabs,TrickExecutor::native_fingerflip,
        {{{BindingPattern::fingerflip,TrickDirection::down,1,ArcadeTrick::finger_30,BindingContext::air,4},{BindingPattern::fingerflip,TrickDirection::up,1,ArcadeTrick::finger_30,BindingContext::air,7}}},2,0,0,0,{},0,rc3_evidence,{30,6,56,4,25}}
};
for(auto& t:entries)t.evidence=apply_trick_evidence(t.id,t.evidence);
return entries;
}();
struct GrindBindingPair { std::int16_t first{},second{}; std::uint8_t target{}; };
inline constexpr auto grind_change_bindings=[] {
    std::array<GrindBindingPair,9> out{};
    for(const auto& t:trick_registry)for(unsigned i=0;i<t.binding_count;++i) {
        const auto& b=t.bindings[i];if(b.pattern==BindingPattern::rail_change && b.parameter>=1 && b.parameter<=out.size())
            out[b.parameter-1]={b.first_button,b.second_button,static_cast<std::uint8_t>(b.parameter)};
    }
    return out;
}();
constexpr const TrickDefinition* trick_definition(ArcadeTrick id) noexcept {
    for (const auto& trick : trick_registry) {
        if (trick.id == id) return &trick;
        for (unsigned i=0;i<trick.grab_variant_count;++i) if (trick.grab_variants[i].trick==id) return &trick;
    }
    return nullptr;
}
constexpr std::string_view trick_name(ArcadeTrick id) noexcept {
    for(const auto& selector:rail_selectors)if(selector.request==id)return "Rail approach selection";
    const auto* trick=trick_definition(id); return trick?trick->name:std::string_view{};
}
constexpr std::string_view grind_change_name(GrindChangeTarget target) noexcept {
    for(const auto& t:trick_registry)for(unsigned i=0;i<t.binding_count;++i)
        if(t.bindings[i].pattern==BindingPattern::rail_change && t.bindings[i].parameter==grind_change_variant(target))return t.name;
    return {};
}
constexpr unsigned held_flip_base(ArcadeTrick id) noexcept {
    const auto* trick=trick_definition(id);return trick?trick->held_base:0;
}
#ifdef DINGOSDK_ARCADE_GAMEPLAY_FINGERPRINT
inline constexpr std::string_view gameplay_fingerprint=DINGOSDK_ARCADE_GAMEPLAY_FINGERPRINT;
#else
inline constexpr std::string_view gameplay_fingerprint{};
#endif
constexpr TrickStatus trick_status(const TrickDefinition& trick,unsigned game_build=20260929,
                                  std::string_view implementation=gameplay_fingerprint) noexcept {
    if(trick.executor==TrickExecutor::none)return TrickStatus::planned;
    const auto& e=trick.evidence;
    unsigned observed{};
    for(const auto& item:e.observations)if(!item.invalidated && item.kind==EvidenceKind::native_gameplay &&
        item.repeat_count>=2 && !item.capture.empty() && !item.observation.empty())
        observed|=static_cast<unsigned>(item.criterion);
    if(!e.invalidated && e.game_build==game_build && e.revision.size()==40 && !implementation.empty() && e.implementation_fingerprint==implementation &&
       !e.context.empty() && !e.reference.empty() && e.required && (observed&e.required)==e.required)
        return TrickStatus::verified;
    return TrickStatus::experimental;
}
constexpr const TrickBinding* binding_for(ArcadeTrick id,BindingPattern pattern) noexcept {
    const auto* trick=trick_definition(id);if(!trick)return nullptr;
    for(unsigned i=0;i<trick->binding_count;++i)if(trick->bindings[i].pattern==pattern)return &trick->bindings[i];
    return nullptr;
}
constexpr std::int16_t direction_code(TrickDirection direction) noexcept {
    constexpr std::array<std::int16_t,10> codes{0,4,8,6,2,7,9,1,3,0};
    return codes[static_cast<unsigned>(direction)];
}
constexpr ArcadeTrick extended_grab_binding(std::span<const TrickDefinition> catalog,std::int16_t direction,ArcadeTrick selected) noexcept {
    for(const auto& t:catalog)for(unsigned i=0;i<t.binding_count;++i) {
        const auto& b=t.bindings[i];
        if(b.pattern==BindingPattern::extended_grab && direction_code(b.direction)==direction)return b.variant;
    }
    return selected;
}
constexpr ArcadeTrick manual_pair_trick(int first,int second) noexcept {
    if(!first || first!=-second)return ArcadeTrick::none;
    for(const auto& t:trick_registry)for(unsigned i=0;i<t.binding_count;++i) {
        const auto& b=t.bindings[i];
        if(b.pattern==BindingPattern::manual_pair &&
           b.direction==(first>0?TrickDirection::up:TrickDirection::down))return t.id;
    }
    return ArcadeTrick::none;
}
constexpr ArcadeTrick neutral_flip_trick() noexcept {
    for(const auto& t:trick_registry)for(unsigned i=0;i<t.binding_count;++i)
        if(t.bindings[i].pattern==BindingPattern::neutral_flip)return t.id;
    return ArcadeTrick::none;
}
constexpr unsigned fingerflip_input(TrickDirection direction) noexcept {
    for(const auto& t:trick_registry)for(unsigned i=0;i<t.binding_count;++i) {
        const auto& b=t.bindings[i];if(b.pattern==BindingPattern::fingerflip && b.direction==direction)return b.parameter;
    }
    return 0;
}
constexpr unsigned ground_grab_variant(TrickDirection direction) noexcept {
    for(const auto& trick:trick_registry)for(unsigned i=0;i<trick.binding_count;++i) {
        const auto& b=trick.bindings[i];if(b.pattern==BindingPattern::ground_grab && b.direction==direction)return b.parameter;
    }
    return 0;
}
constexpr ArcadeTrick select_registry_trick(std::span<const TrickDefinition> catalog,TrickFamily family,
                                           TrickDirection direction,unsigned presses) noexcept {
    if(family==TrickFamily::grind && presses==1 && direction!=TrickDirection::none)
        for(const auto& selector:rail_selectors)for(const auto d:selector.directions)
            if(d!=TrickDirection::invalid && d==direction)return selector.request;
    const auto category=family==TrickFamily::flip?TrickCategory::flips:family==TrickFamily::grab?TrickCategory::grabs:TrickCategory::grinds;
    for(const auto& t:catalog)if(t.category==category)for(unsigned i=0;i<t.binding_count;++i) {
        const auto& b=t.bindings[i];
        if(b.pattern==BindingPattern::named && b.direction==direction && b.presses==presses)return b.variant;
    }
    return ArcadeTrick::none;
}
}
