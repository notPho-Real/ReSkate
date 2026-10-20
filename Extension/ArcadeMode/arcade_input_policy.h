#pragma once
#include <cstdint>
namespace dingosdk::arcade_mode {
struct InputTiming {
    static constexpr std::uint64_t ground_pair_ms = 220;
    static constexpr std::uint64_t ground_sample_slack_ms = 55;
    static constexpr std::uint64_t air_pair_ms = 400, air_request_ms = 700;
    static constexpr std::uint64_t direction_ms = 500, extra_press_ms = 300;
    static constexpr std::uint64_t takeoff_preload_ms = 160, readiness_ms = 200;
    static constexpr std::uint64_t native_sample_lease_ms = 150;
};
struct InputGenerations {
    std::uint64_t owner{}, contact{}, action{};
    bool operator==(const InputGenerations&) const = default;
};
struct InputAirEpisode {
    std::uint64_t number{}, departure{};
    bool operator==(const InputAirEpisode&) const = default;
};
// Existing read-only native selection observation. It is not an installed
// input-table epoch, animation clock or permission to start an actor.
struct InputFlipObservation {
    bool valid{};
    std::int32_t id{-1};
    std::uintptr_t object{};
    std::uint32_t state{}, mode{}, stance{};
    std::uint64_t step{}, mode_generation{};
    bool operator==(const InputFlipObservation&) const = default;
};
// Correlation from an independently sampled native producer, never a trick
// occurrence or animation clock. Polling cannot renew the producer's lease.
struct InputNativeContext {
    bool required{}, valid{};
    InputGenerations generations{};
    std::uint64_t physics_step{}, sampled_at{};
    bool rail_family{};
    // Physics-observed flight and departure identities, never action or
    // animation completion. A departure need not be a landing.
    bool airborne{};
    std::uint64_t airborne_episode{}, air_departure{};
    InputFlipObservation flip{};
    InputAirEpisode air_episode() const noexcept { return {airborne_episode,air_departure}; }
    bool operator==(const InputNativeContext&) const = default;
    bool fresh(std::uint64_t now) const noexcept {
        return valid && generations.owner && generations.contact && generations.action && physics_step &&
            now >= sampled_at && now - sampled_at <= InputTiming::native_sample_lease_ms;
    }
    // Only the existing201/mode2 jump1-to-idle observation is correlated here.
    // This proves neither actor completion nor current readiness.
    bool jump_selection_retired(InputGenerations origin, InputAirEpisode episode,
                                const InputFlipObservation& before) const noexcept {
        return valid && airborne && airborne_episode && air_episode() == episode &&
            generations.owner == origin.owner && generations.action > origin.action &&
            generations.action - origin.action == 1 && before.valid && flip.valid &&
            before.id == 1 && flip.id == -1 && before.object && before.object == flip.object &&
            before.state == 201 && before.state == flip.state && before.mode == 2 &&
            before.mode == flip.mode && before.stance == flip.stance && before.step && flip.step > before.step &&
            before.mode_generation && before.mode_generation == flip.mode_generation;
    }
    bool correlates(InputGenerations publication) const noexcept {
        return !required || (valid && generations.owner && generations.contact &&
            publication.owner == generations.owner && publication.contact == generations.contact);
    }
    bool correlates_action(InputGenerations publication) const noexcept {
        return correlates(publication) && (!required || publication.action == generations.action);
    }
    bool follows(const InputNativeContext& previous) const noexcept {
        if (!previous.valid) return true;
        if (physics_step < previous.physics_step || sampled_at < previous.sampled_at) return false;
        if (generations.owner != previous.generations.owner)
            return generations.owner > previous.generations.owner && physics_step > previous.physics_step;
        return physics_step >= previous.physics_step && sampled_at >= previous.sampled_at &&
            generations.contact >= previous.generations.contact && generations.action >= previous.generations.action &&
            airborne_episode >= previous.airborne_episode && air_departure >= previous.air_departure &&
            (physics_step != previous.physics_step || *this == previous);
    }
    template<class Frame>
    static InputNativeContext from_frame(const Frame& frame) noexcept {
        InputNativeContext context{true,frame.valid,{frame.owner_generation,frame.contact_generation,frame.action_generation},
            frame.physics_step,frame.sampled_at,frame.rail_family};
        if constexpr (requires { frame.airborne; frame.airborne_episode; frame.air_departure; }) {
            context.airborne = frame.airborne; context.airborne_episode = frame.airborne_episode;
            context.air_departure = frame.air_departure;
        }
        if constexpr (requires { frame.flip_known; frame.flip_id; frame.selection_object;
                                 frame.state; frame.mode; frame.stance_and_push; }) {
            context.flip = {frame.flip_known,frame.flip_id,frame.selection_object,
                frame.state,frame.mode,frame.stance_and_push,frame.physics_step};
            if constexpr (requires { frame.mode_generation; }) context.flip.mode_generation = frame.mode_generation;
        }
        return context;
    }
};
}

namespace dingosdk::arcade_mode {
enum class InputNormalization : std::uint8_t { compatibility, analog };
enum class InputBoardPattern : std::uint8_t { shoulder_overlap, single_control, unavailable };
struct InputBoardBinding {
    InputBoardPattern pattern{};
    std::uint16_t control_mask{0x0300};
    bool operator==(const InputBoardBinding&) const = default;
};
struct InputPolicy {
    InputNormalization normalization{InputNormalization::analog};
    std::uint64_t manual_sample_allowance{InputTiming::ground_sample_slack_ms};
    bool inclusive_manual{true}, reject_opposing{true}, bounded_handoff{false};
    bool operator==(const InputPolicy&) const = default;
    // Preserve the admitted physical Manual baseline and its explicit allowance.
    static constexpr InputPolicy production() noexcept { return {}; }
    static constexpr InputPolicy compatibility() noexcept {
        return {InputNormalization::compatibility,InputTiming::ground_sample_slack_ms,true,true,false};
    }
    static constexpr InputPolicy strict() noexcept {
        return {InputNormalization::analog, 0, false, false, false};
    }
};
}

namespace dingosdk::arcade_mode {
enum class InputRailPhase : std::uint8_t { unavailable, approach, early, established, lip_selection, lip_established };
struct InputRailState {
    InputRailPhase phase{};
    std::uint64_t generation{}, gameframe{}, selection_frame{};
    unsigned excluded_variant{}, lip_table{};
    bool fresh{}, ready{}, geometry_known{}, right{}, parallel{}, backwards{}, regular{}, frontside{};
    InputGenerations generations{};
    constexpr std::uint8_t geometry_index() const noexcept {
        return static_cast<std::uint8_t>((right ? 1u : 0u) | (parallel ? 2u : 0u) |
            (backwards ? 4u : 0u) | (regular ? 8u : 0u));
    }
};
}

namespace dingosdk::arcade_mode {
struct InputSurfaceState {
    std::uint64_t generation{}, physics_step{};
    bool fresh{}, can_plant{}, can_push{}, can_transfer{};
    std::uint16_t transfer_buttons{}; // Zero keeps an unselected platform binding unavailable.
    InputGenerations generations{};
};
}
