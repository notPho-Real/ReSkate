#pragma once
#include "arcade_manual_input.h"
#include "arcade_action_dispatch.h"
#include "arcade_input_dispatch.h"
#include "arcade_launch_descriptor.h"
#include "arcade_footplant.h"
#include "arcade_ground_grab.h"
#include "arcade_authored_playback.h"
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
    // Observation only: entered=1, unavailable=2, authored=4, manual=8,
    // auxiliary=16, rail=32, extension=64. Never grants native admission.
    unsigned grab_gate{};
    // Current-poll delivery receipt from the qualified native grab lease.
    // This is channel ownership, never native animation acknowledgment.
    bool native_grab_triggers{};
    bool native_grab_selector{};
    bool native_grab_feet{};
    bool blocks_air_rotation(bool native_held_shoulder) const noexcept {
        // Only the existing single-held-shoulder native LS actuator may share
        // native grab controls. A qualified family selector owns RS separately;
        // tap/directional body-rate writers and other RS owners stay excluded.
        const bool selector = native_grab_selector && pad.owns_right_stick &&
            ((pad.right_x==28000 && pad.right_y==0) || (pad.right_x==0 && (pad.right_y==28000 || pad.right_y==-28000)));
        const bool feet = native_grab_feet && pad.owns_face_buttons &&
            (pad.face_buttons==0x1000 || pad.face_buttons==0x2000 || pad.face_buttons==0x4000);
        const bool grab_controls = native_grab_triggers && pad.owns_triggers &&
            !pad.owns_left_stick && (!pad.owns_right_stick || selector) && (!pad.owns_face_buttons || feet) &&
            (pad.left_trigger==0 || pad.left_trigger==255) &&
            (pad.right_trigger==0 || pad.right_trigger==255) && (pad.left_trigger || pad.right_trigger);
        return owned && !(native_held_shoulder && grab_controls);
    }
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
    bool (*continues)(void*,InputGenerations,InputAirEpisode) noexcept{};
    unsigned (*hand_inputs)(void*) noexcept{}; // Native left/right trigger mask; zero withdraws both.
    native_grab::Control (*controls)(void*) noexcept{}; // One qualified lease snapshot for hands and family selector.
    ArcadeTrick selected{ArcadeTrick::double_grab}; // R3+B neutral selection, captured with the physical event.
    bool (*fingerflip)(void*,std::uint8_t,std::span<const AuthoredStickStep>,std::uint64_t,bool) noexcept{};
};
struct NativeFootplantRequest {
    void* context{};
    bool (*submit)(void*,Footplant) noexcept{};
    FootplantControl (*controls)(void*) noexcept{};
};
struct NativeGroundGrabRequest {
    void* context{};
    bool (*submit)(void*,GroundGrab) noexcept{};
    GroundGrabControl (*controls)(void*,bool held) noexcept{};
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
        std::int32_t held_flip_request{}; // Physical repeat accepted by input playback, not native execution.
        bool held_input_released{},held_actor_acknowledged{};
        bool footplant_owned{};
        bool ground_grab_owned{};
    };
};
}
