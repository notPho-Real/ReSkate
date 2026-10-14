#pragma once
#include "arcade_action_dispatch.h"
#include "arcade_aux_controls.h"
#include "arcade_combo_session.h"

namespace dingosdk::arcade_mode {
enum class ActionExperiment : unsigned { off, observe, grabs, revert_frontside, revert_backside };
struct ActionPhysicalPad {
    std::uint16_t buttons{};
    short left_x{}, left_y{};
    std::uint8_t left_trigger{}, right_trigger{};
};
// Unsaved lab adapter. Production defaults remain unchanged until native tests.
// All channels begin with the caller's ordinary remap; ownership is explicit.
class ActionRuntime {
public:
    struct Output {
        ActionDispatch::Output pad{};
        std::uint16_t consume_buttons{};
        bool consume_rt{}, requested{}, owned{};
        ArcadeTrick trick{ArcadeTrick::none};
        unsigned request_kind{}; // 1 grab, 2 grind, 3 frontside revert, 4 backside revert
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
        const bool revert_mode = mode == ActionExperiment::revert_frontside || mode == ActionExperiment::revert_backside;
        out.consume_buttons = mode == ActionExperiment::grabs ? 0x2000 : 0;
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
            true, conflict);
        if (request.ready) {
            out.requested = true; out.trick = request.trick;
            out.request_kind = request.family == TrickFamily::grab ? 1 : 2;
            if (request.family == TrickFamily::grab && mode == ActionExperiment::grabs)
                out.result = dispatch_.request_grab(request.trick, now, context);
            else if (request.family == TrickFamily::grind && mode == ActionExperiment::observe)
                out.result = DispatchResult::needs_geometry;
        }
        if (rt_edge && revert_mode) {
            out.requested = true;
            out.request_kind = mode == ActionExperiment::revert_frontside ? 3 : 4;
            out.result = !inspected_stance ? DispatchResult::unsafe_context : dispatch_.request_revert(mode == ActionExperiment::revert_frontside ?
                RevertSide::frontside : RevertSide::backside,
                {physical.left_trigger > 30, false, false}, now, context);
        }
        if (!inspected_stance) dispatch_.cancel();
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
