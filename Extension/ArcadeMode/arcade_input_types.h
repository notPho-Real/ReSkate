#pragma once
#include "arcade_manual_input.h"
#include "arcade_action_dispatch.h"
namespace dingosdk::arcade_mode {
enum class NativeTrick : unsigned { none, ollie, kickflip, heelflip };
enum class ActionExperiment : unsigned { off, observe, grabs, revert_frontside, revert_backside, classic };
inline constexpr ActionExperiment default_action_mode = ActionExperiment::classic;
struct ActionPhysicalPad {
    std::uint16_t buttons{};
    std::int16_t left_x{}, left_y{};
    std::uint8_t left_trigger{}, right_trigger{};
};
struct ActionInputOutput {
    ActionDispatch::Output pad{};
    std::uint16_t consume_buttons{};
    bool consume_rt{}, requested{}, owned{};
    ArcadeTrick trick{ArcadeTrick::none};
    unsigned request_kind{};
    DispatchResult result{DispatchResult::disabled};
};
// Output contracts only; all production recognition belongs to InputSession.
struct NativeGesture {
    static constexpr std::uint16_t a = 0x1000, x = 0x4000;
    struct Output {
        std::uint16_t buttons{};
        std::int16_t right_x{}, right_y{};
        NativeTrick started{};
        unsigned phase{};
        std::uint8_t requested_id{};
        bool unsupported{}, authored_playback{}, command_busy{}, suppress_reverse{}, movement_intent{};
        ManualInputDiagnostic manual{};
        ActionInputOutput action{};
    };
};
}
