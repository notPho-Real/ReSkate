#pragma once
#include "arcade_action_dispatch.h"
#include "arcade_aux_controls.h"
#include "arcade_combo_session.h"

namespace dingosdk::arcade_mode {
enum class ActionExperiment : unsigned { off, observe, grabs, revert_frontside, revert_backside, classic };
inline constexpr ActionExperiment default_action_mode = ActionExperiment::classic;
struct ActionPhysicalPad {
    std::uint16_t buttons{};
    short left_x{}, left_y{};
    std::uint8_t left_trigger{}, right_trigger{};
};
// Classic RT uses the verified moving-context recipe; lab selectors stay unsaved.
// All channels begin with the caller's ordinary remap; ownership is explicit.
class ActionRuntime {
public:
    struct Output {
        ActionDispatch::Output pad{};
        std::uint16_t consume_buttons{};
        bool consume_rt{}, requested{}, owned{};
        ArcadeTrick trick{ArcadeTrick::none};
        unsigned request_kind{}; // 1 grab, 2 grind, 3/4 revert recipe direction; never observed side
        DispatchResult result{DispatchResult::disabled};
    };
    void reset() noexcept {
        dispatch_.cancel(); controls_.cancel(); armed_ = false; previous_rt_ = false; clocked_ = false;
    }
    bool busy() const noexcept { return dispatch_.busy(); }
    Output poll(ActionExperiment mode, std::uint64_t now, const ActionPhysicalPad& physical,
                bool active, bool fresh, SkatePose pose, bool inspected_stance, bool other_owner) noexcept {
        Output out{};
        if (!active || !fresh || mode == ActionExperiment::off || (clocked_ && now < last_)) { reset(); return out; }
        clocked_ = true; last_ = now;
        const bool classic = mode == ActionExperiment::classic;
        const bool revert_mode = classic || mode == ActionExperiment::revert_frontside || mode == ActionExperiment::revert_backside;
        const bool unverified_nollie = classic && physical.left_trigger > 30;
        out.consume_buttons = mode == ActionExperiment::grabs || (classic && pose == SkatePose::air) ? 0x2000 : 0;
        out.consume_rt = revert_mode;
        const bool rt = physical.right_trigger > 128;
        const bool held_command = (physical.buttons & 0xa000) || rt;
        if (!armed_) {
            if (!held_command) armed_ = true;
            previous_rt_ = rt;
            controls_.poll(now, 0, 0, 0, true, false);
            return out;
        }
        const bool rt_edge = rt && !previous_rt_;
        previous_rt_ = rt;
        // Existing authored gestures/manuals win ownership; shoulders remain the
        // established spin/board path. Camera movement is deliberately no conflict.
        const bool conflict = other_owner || (physical.buttons & (0x5000 | 0x0300 | 0x0010));
        ActionContext context{mode != ActionExperiment::observe, fresh, pose == SkatePose::air,
            pose == SkatePose::riding, inspected_stance, conflict};
        const auto request = controls_.poll(now, physical.buttons, physical.left_x, physical.left_y,
            true, conflict, RailRelationship::unknown, classic);
        if (request.ready) {
            out.requested = true; out.trick = request.trick;
            // Normal B currently exposes the independently verified basic grab.
            // Movement held during a jump must not turn it into an unavailable
            // named modifier. Keep the actual basic identity in diagnostics;
            // the explicit lab selector still requests the named catalog entry.
            if (classic && request.family == TrickFamily::grab && request.presses == 1 &&
                request.direction != TrickDirection::invalid)
                out.trick = ArcadeTrick::basic_bs_grab;
            out.request_kind = request.family == TrickFamily::grab ? 1 : 2;
            if (request.family == TrickFamily::grab && (mode == ActionExperiment::grabs || classic)) {
                // A native BS Grab is not a successful Melon. Preserve candidate
                // recipes for the explicit lab selector without substituting them
                // for unproved named controls during normal play.
                out.result = classic && out.trick != ArcadeTrick::basic_bs_grab
                    ? DispatchResult::unavailable : dispatch_.request_grab(out.trick, now, context);
            }
            else if (request.family == TrickFamily::grind && mode == ActionExperiment::observe)
                out.result = DispatchResult::needs_geometry;
        }
        if (rt_edge && revert_mode) {
            out.requested = true;
            const bool backside_recipe = mode == ActionExperiment::revert_backside;
            out.request_kind = backside_recipe ? 4 : 3;
            out.result = !inspected_stance || unverified_nollie ? DispatchResult::unsafe_context : dispatch_.request_revert(
                backside_recipe ? RevertSide::backside : RevertSide::frontside,
                {physical.left_trigger > 30, false, false}, now, context);
        }
        if (!inspected_stance || unverified_nollie) dispatch_.cancel();
        out.pad = dispatch_.sample(now, context);
        out.owned = out.pad.owns_right_stick || out.pad.owns_left_stick || out.pad.owns_triggers || out.pad.owns_face_buttons;
        return out;
    }
private:
    ActionDispatch dispatch_;
    AuxiliaryControls controls_;
    bool armed_{}, previous_rt_{}, clocked_{};
    std::uint64_t last_{};
};
}
