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
