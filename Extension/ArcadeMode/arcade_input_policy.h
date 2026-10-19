#pragma once
#include <cstdint>
namespace dingosdk::arcade_mode {
struct InputTiming {
    static constexpr std::uint64_t ground_pair_ms = 220;
    static constexpr std::uint64_t ground_sample_slack_ms = 55;
    static constexpr std::uint64_t air_pair_ms = 400, air_request_ms = 700;
    static constexpr std::uint64_t direction_ms = 500, extra_press_ms = 300;
    static constexpr std::uint64_t takeoff_preload_ms = 160, readiness_ms = 200;
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
    InputNormalization normalization{InputNormalization::compatibility};
    std::uint64_t manual_sample_allowance{InputTiming::ground_sample_slack_ms};
    bool inclusive_manual{true}, reject_opposing{true}, bounded_handoff{false};
    bool operator==(const InputPolicy&) const = default;
    // Preserve the admitted physical Manual baseline and its explicit allowance.
    static constexpr InputPolicy production() noexcept { return {}; }
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
};
}
