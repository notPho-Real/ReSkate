#pragma once
#include "arcade_input_dispatch.h"
#include "arcade_input_types.h"
#include "arcade_classic_input.h"
#include "arcade_push.h"
#include "arcade_board.h"

namespace dingosdk::arcade_mode {
struct InputAdmission {
    bool gameplay{}, may_start{}, may_push{}, may_toggle_board{}, authored{};
    bool nollie{}, mirror_x{}, manual_safe{}, manual_prepare{}, offboard{}, external_owner{};
    bool flip_ready{true}, neutral_ready{}, action_fresh{}, inspected_stance{};
    SkatePose pose{SkatePose::unknown};
    ActionExperiment action_mode{ActionExperiment::off};
    float speed{};
    std::uint64_t tick{};
    std::span<const PreparedGesture> catalog{};
    unsigned observed_balance_branch{};
    bool observed_lip{}; // Reserved for independently verified native contact admission.
};
// One per-controller journal and one dispatch path. Native playback objects have
// no physical edge buffers; their only inputs are admitted semantic requests.
class InputSession {
public:
    void reset() noexcept {
        input_.reset(); authored_.cancel(); manual_.cancel(); auxiliary_.cancel();
        push_.reset(); board_.reset(); rail_ = holding_ = air_action_ = false;
        clocked_ = false; current_ = nullptr;
    }
    void cancel_manual() noexcept { manual_.cancel(); input_.reset_manual_continuity(); }
    void set_schedule(AuthoredPlaybackSchedule schedule) noexcept {
        if (authored_.schedule() == schedule) return;
        reset(); authored_.set_schedule(schedule);
    }
    bool busy() const noexcept { return auxiliary_.busy() || authored_.busy() || manual_.busy() || rail_; }
    const UnifiedInput::Diagnostic& diagnostic() const noexcept { return input_.diagnostic(); }
    const InputHistory& history() const noexcept { return input_.history(); }
    InputRequest pending_manual() const noexcept { return input_.pending_manual(); }
    NativeGesture::Output poll(std::uint64_t now, PhysicalInputSample physical,
                              std::int16_t rx, std::int16_t ry, InputAdmission admission) noexcept {
        NativeGesture::Output output{physical.buttons, rx, ry};
        if (!admission.gameplay || admission.offboard || admission.pose == SkatePose::bail ||
            (admission.pose == SkatePose::unknown && !admission.may_start && !manual_.busy()) ||
            (clocked_ && (now < last_ || admission.tick < last_tick_))) {
            reset();
            if (admission.gameplay && admission.offboard) output.right_x = output.right_y = 0;
            return output;
        }
        clocked_ = true; last_ = now; last_tick_ = admission.tick;
        now_ = now; physical_ = physical; admission_ = admission; current_ = &output;
        output.buttons &= static_cast<std::uint16_t>(~(classic_a | classic_x | 15u | 0x8000u));
        output.right_x = output.right_y = 0; // Physical right stick belongs to the camera.
        authored_.finish_neutral_tail();
        if (air_action_ && admission.pose != SkatePose::air) { authored_.cancel(); air_action_ = false; }
        if (admission.external_owner || auxiliary_.busy()) { authored_.cancel(); holding_ = false; }
        if (!(physical.buttons & 0x8000u)) rail_ = false;
        const bool triggers = physical.lt > 30 || physical.rt > 30;
        const bool manual_was_active = manual_.busy();
        auto manual = manual_.sample(now, true, admission.manual_safe, admission.pose,
            authored_.busy() || auxiliary_.busy() || admission.external_owner || triggers, physical.buttons);
        const bool reset_manual = manual_was_active && !manual_.busy() && manual_.hard_reset();
        const InputContext context = admission.observed_lip ? InputContext::lip : admission.pose == SkatePose::air ? InputContext::air :
            admission.pose == SkatePose::manual || admission.pose == SkatePose::nose_manual ? InputContext::balance :
            admission.pose == SkatePose::grind ? InputContext::rail : InputContext::ground;
        const bool action_enabled = admission.action_mode != ActionExperiment::off;
        if (!admission.action_fresh || !action_enabled) auxiliary_.cancel();
        output.action.consume_buttons = admission.action_fresh &&
            (admission.action_mode == ActionExperiment::grabs ||
             (admission.action_mode == ActionExperiment::classic && context == InputContext::air)) ? 0x2000 : 0;
        output.action.consume_rt = admission.action_fresh &&
            (admission.action_mode == ActionExperiment::classic || admission.action_mode == ActionExperiment::revert_frontside ||
             admission.action_mode == ActionExperiment::revert_backside);
        const bool manual_conflict = authored_.busy() || auxiliary_.busy() || admission.external_owner || triggers;
        InputEnvironment environment{context, true, admission.external_owner, admission.nollie, admission.observed_balance_branch,
            admission.manual_safe && !admission.external_owner && !triggers,
            admission.manual_prepare && !admission.external_owner && !triggers, manual_.busy() || reset_manual,
            action_enabled && admission.action_fresh && !auxiliary_.busy(), !authored_.busy() && !auxiliary_.busy(), admission.may_start,
            admission.pose == SkatePose::riding || admission.pose == SkatePose::air || manual_.busy(),
            admission.action_mode == ActionExperiment::grabs};
        input_.poll(now, physical, environment, {this, &receive});
        if (reset_manual) input_.reset_manual_continuity();
        // A released A ends native setup even when another request superseded
        // recognition in this poll. No new edge or jump is synthesized here.
        if (holding_ && !(physical.buttons & classic_a)) { authored_.release(admission.tick); holding_ = false; }
        const auto authored = authored_.sample(admission.tick, now);
        if (authored.owns_stick) {
            output.right_x = authored.x; output.right_y = authored.y; output.phase = 1;
            output.authored_playback = true;
        }
        manual = manual_.sample(now, true, admission.manual_safe, admission.pose,
            authored_.busy() || auxiliary_.busy() || admission.external_owner || triggers, physical.buttons);
        if (manual.owns_stick) { output.right_x = 0; output.right_y = manual.y; output.phase = 1; }
        const auto& diagnostic = input_.diagnostic();
        output.suppress_reverse = !manual_.busy() && diagnostic.suppress_reverse;
        output.manual = {diagnostic.stick, diagnostic.pad,
            diagnostic.stick_first, diagnostic.pad_first, manual_.requested(), diagnostic.stick_at,
            diagnostic.pad_at, manual_.requested_at(),
            !admission.manual_safe && !admission.manual_prepare ? 4u : manual_conflict ? 2u : 0u,
            admission.manual_safe, admission.manual_prepare, manual.owns_stick, manual.confirmed};
        bool busy = authored_.busy() || manual_.busy() || input_.pending_stick();
        if (rail_ && !busy && !triggers && !auxiliary_.busy() &&
            (context == InputContext::air || context == InputContext::rail)) {
            output.right_x = 0;
            output.right_y = rail_direction_ == TrickDirection::up ? 20000 : rail_direction_ == TrickDirection::down ? -20000 : 0;
            output.phase = 1; busy = true;
        }
        ActionContext action_context{admission.action_mode != ActionExperiment::observe, admission.action_fresh,
            context == InputContext::air, context == InputContext::ground, admission.inspected_stance,
            busy || admission.external_owner || (physical.buttons & (0x5000u | 0x0300u | 0x0010u)) != 0};
        if (!admission.inspected_stance || (admission.action_mode == ActionExperiment::classic && admission.nollie)) auxiliary_.cancel();
        output.action.pad = auxiliary_.sample(now, action_context);
        const auto& action = output.action.pad;
        output.action.owned = action.owns_right_stick || action.owns_left_stick || action.owns_triggers || action.owns_face_buttons;
        const auto board = board_.poll(now, output.buttons, true,
            admission.may_toggle_board && !busy && !output.action.owned && !triggers && !(physical.buttons & ~BoardChord::pair));
        output.buttons = board.buttons;
        if (board.pulse) output.phase = 3;
        const bool conflict = physical.buttons || triggers || busy || output.action.owned || admission.external_owner;
        if (push_.poll(now, physical.x, physical.y, admission.speed, true, admission.may_push && !conflict))
            output.buttons |= NativeGesture::x;
        output.movement_intent = push_.movement_intent();
        output.command_busy = busy || output.action.owned || board.pulse;
        current_ = nullptr;
        return output;
    }
private:
    static TrickDirection direction(std::int16_t code) noexcept {
        switch (code) {
        case 1: return TrickDirection::down_left; case 2: return TrickDirection::down; case 3: return TrickDirection::down_right;
        case 4: return TrickDirection::left; case 6: return TrickDirection::right;
        case 7: return TrickDirection::up_left; case 8: return TrickDirection::up; case 9: return TrickDirection::up_right;
        default: return TrickDirection::none;
        }
    }
    static InputDisposition receive(void* self, const InputRequest& request) noexcept {
        return static_cast<InputSession*>(self)->request(request);
    }
    InputDisposition request(const InputRequest& request) noexcept {
        if (!current_) return InputDisposition::cancelled;
        auto& out = *current_;
        const auto& admission = admission_;
        const bool airborne = admission.pose == SkatePose::air;
        const auto context = admission.pose == SkatePose::grind ? GestureContext::from_grind : launch_context_;
        switch (request.action) {
        case InputAction::crouch:
            if (!admission.may_start || authored_.busy() || auxiliary_.busy() || manual_.busy()) return InputDisposition::cancelled;
            launch_context_ = admission.pose == SkatePose::grind ? GestureContext::from_grind : GestureContext::ground;
            holding_ = start(request.nollie ? 16 : 1, request.nollie, true, launch_context_, false);
            return holding_ ? InputDisposition::started : InputDisposition::unsupported;
        case InputAction::ollie_release:
            authored_.release(admission.tick); holding_ = false; return InputDisposition::started;
        case InputAction::flip:
            if (!airborne) return InputDisposition::cancelled;
            if (!admission.flip_ready || authored_.busy() || auxiliary_.busy()) return InputDisposition::waiting;
            {
                const auto id = native_flip_id(select_named_trick(TrickFamily::flip, direction(request.direction), request.presses), request.nollie);
                air_action_ = start(id, request.nollie, false, context, true);
                return air_action_ ? InputDisposition::started : InputDisposition::unsupported;
            }
        case InputAction::manual: case InputAction::nose_manual:
            if (!admission.manual_safe || admission.external_owner || physical_.lt > 30 || physical_.rt > 30 ||
                (physical_.buttons & 0xf030u) || (physical_.buttons & 0x0300u) == 0x0300u) return InputDisposition::cancelled;
            if (request.context == InputContext::air && now_ - request.recognized_at >= InputTiming::air_request_ms)
                return InputDisposition::expired; // Existing native preparation deadline is half-open.
            if (authored_.busy() || auxiliary_.busy()) return InputDisposition::waiting;
            return manual_.request(now_, request.action == InputAction::manual ? -1 : 1, airborne, admission.manual_safe,
                request.context == InputContext::air, request.recognized_at)
                ? InputDisposition::started : InputDisposition::cancelled;
        case InputAction::rail_request:
            if (!(physical_.buttons & 0x8000u)) return InputDisposition::cancelled;
            rail_direction_ = direction(request.direction);
            if (rail_direction_ != TrickDirection::none && rail_direction_ != TrickDirection::up && rail_direction_ != TrickDirection::down) {
                out.unsupported = true; return InputDisposition::unsupported;
            }
            rail_ = true; return InputDisposition::started;
        case InputAction::grab: case InputAction::revert:
            return request_auxiliary(request);
        case InputAction::balance_branch_one: case InputAction::balance_branch_two: case InputAction::balance_variant: case InputAction::lip_request: case InputAction::balance_return:
            out.unsupported = true; return InputDisposition::unsupported;
        default: return InputDisposition::unsupported;
        }
    }
    InputDisposition request_auxiliary(const InputRequest& request) noexcept {
        auto& out = current_->action;
        const auto& admission = admission_;
        const bool grab = request.action == InputAction::grab;
        const bool classic = admission.action_mode == ActionExperiment::classic;
        if (!admission.action_fresh || admission.action_mode == ActionExperiment::off) return InputDisposition::cancelled;
        if (authored_.busy() || manual_.busy() || auxiliary_.busy() || rail_) return InputDisposition::waiting;
        out.requested = true; out.request_kind = grab ? 1u : admission.action_mode == ActionExperiment::revert_backside ? 4u : 3u;
        ActionContext context{admission.action_mode != ActionExperiment::observe, true,
            admission.pose == SkatePose::air, admission.pose == SkatePose::riding, admission.inspected_stance,
            admission.external_owner || (physical_.buttons & (0x5000u | 0x0300u | 0x0010u)) != 0};
        if (grab) {
            out.trick = classic && request.presses == 1 ? ArcadeTrick::basic_bs_grab :
                select_named_trick(TrickFamily::grab, direction(request.direction), request.presses);
            out.result = classic && (out.trick != ArcadeTrick::basic_bs_grab || admission.nollie) ? DispatchResult::unavailable :
                admission.action_mode == ActionExperiment::grabs || classic ? auxiliary_.request_grab(out.trick, now_, context) : DispatchResult::disabled;
        } else {
            out.result = !admission.inspected_stance || (classic && admission.nollie) ? DispatchResult::unsafe_context :
                auxiliary_.request_revert(admission.action_mode == ActionExperiment::revert_backside ? RevertSide::backside : RevertSide::frontside,
                    {admission.nollie, false, false}, now_, context);
        }
        return out.result == DispatchResult::started ? InputDisposition::started :
            out.result == DispatchResult::busy ? InputDisposition::waiting : InputDisposition::unsupported;
    }
    bool start(std::uint8_t id, bool nollie, bool hold, GestureContext context, bool airborne) noexcept {
        if (!admission_.authored) { current_->unsupported = true; return false; }
        for (const auto& gesture : admission_.catalog) {
            if (!id || gesture.native_id != id || gesture.nollie != nollie || gesture.context != context) continue;
            const auto& steps = airborne ? (admission_.neutral_ready ? gesture.ready_air_steps : gesture.air_steps) : gesture.steps;
            if (authored_.start(steps, admission_.tick, hold, admission_.mirror_x, airborne)) {
                current_->requested_id = id; return true;
            }
        }
        current_->unsupported = true; return false;
    }
    UnifiedInput input_{};
    AuthoredPlayback authored_{};
    ManualActuation manual_{};
    ActionDispatch auxiliary_{};
    AutomaticPush push_{};
    BoardChord board_{};
    InputAdmission admission_{};
    PhysicalInputSample physical_{};
    NativeGesture::Output* current_{};
    GestureContext launch_context_{GestureContext::ground};
    TrickDirection rail_direction_{};
    std::uint64_t now_{}, last_{}, last_tick_{};
    bool rail_{}, holding_{}, air_action_{}, clocked_{};
};
}
