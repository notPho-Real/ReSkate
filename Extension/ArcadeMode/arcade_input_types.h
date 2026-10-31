#pragma once
#include "arcade_manual_input.h"
#include "arcade_action_dispatch.h"
#include "arcade_input_dispatch.h"
#include "arcade_launch_descriptor.h"
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
struct InputJumpEdge {
    bool released{}, setup_admitted{}, nollie{}, from_rail{};
    std::uint64_t pressed_at{}, released_at{};
    LaunchDescriptor launch{};
    bool setup_started{}; // Existing setup accepted; not native occurrence or animation proof.
    unsigned charge_ms() const noexcept {
        const auto elapsed = released_at >= pressed_at ? released_at - pressed_at : 0;
        return elapsed > 200 ? 200u : static_cast<unsigned>(elapsed);
    }
};
// An already recognized held command. Empty output withdraws the handoff.
// Occupied channels belong to other input actors, never the rail candidate.
struct InputRailOutput {
    InputRequest request{};
    bool held{}, blocked{};
    ActionDispatch::Output occupied{};
};
// One recognized physical half-turn command, never a native actor/phase ID.
// The input owner preserves this original observation until cancellation.
struct GroundTurnCommand {
    std::uint64_t serial{}, requested_at{};
    InputGenerations generations{};
    std::uint64_t physics_step{}, sampled_at{};
    bool operator==(const GroundTurnCommand&) const = default;
};
// Output contracts only; all production recognition belongs to InputSession.
struct NativeFlipRequest {
    void* context{};
    bool (*submit)(void*,std::int32_t) noexcept{};
};
struct NativeGrabRequest {
    void* context{};
    bool (*submit)(void*,const InputRequest&,ArcadeTrick,bool) noexcept{};
    bool (*right_hand_input)(void*) noexcept{};
};
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
        InputJumpEdge jump{};
        InputRailOutput rail{};
        GroundTurnCommand ground_turn{};
        std::int32_t native_flip_request{}; // Queued intent, never an animation acknowledgment.
        bool automatic_push{}; // Receipt from AutomaticPush, never inferred from physical X.
    };
};
}
