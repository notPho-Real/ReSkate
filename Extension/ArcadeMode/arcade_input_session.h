#pragma once
#include "arcade_input_dispatch.h"
#include "arcade_input_types.h"
#include "arcade_classic_input.h"
#include "arcade_push.h"
#include "arcade_board.h"
#include "arcade_native_flip_entry.h"

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
    bool observed_lip{}; // Correlated native context veto; never actor/balance/exit admission.
    bool manual_balance_admitted{}; // Independent identity plus active nonfailed balance.
    InputPolicy input_policy{InputPolicy::production()};
    bool branch_ready{true};
    InputRailState rail{};
    InputAdapter extensions{};
    InputSurfaceState surface{};
    bool named_grab_bindings{};
    InputBalanceControls balance_controls{};
    InputBoardBinding board_binding{};
    InputNativeContext native_context{};
    InputGenerations balance_generations{};
    bool export_rail_requests{};
    LaunchDescriptor launch{LaunchDescriptor::ordinary()};
    NativeFlipRequest normal_flip{};
    bool manual_contact_only{}; // Physical admission without matching native history.
};
// One per-controller journal and one dispatch path. Native playback objects have
// no physical edge buffers; their only inputs are admitted semantic requests.
class InputSession {
public:
    void reset() noexcept {
        input_.reset(); authored_.cancel(); manual_.cancel(); auxiliary_.cancel();
        push_.reset(); board_.reset(); board_active_ = false; board_consumed_ = 0; rail_ = holding_ = air_action_ = false; air_followup_id_=0;
        clocked_ = false; extension_started_ = false; grab_hold_button_ = 0; current_ = nullptr; jump_launch_ = {};
        air_actor_episode_ = grab_actor_episode_ = manual_air_episode_ = {}; grab_actor_ = false; rail_request_ = {};
        ground_turn_ = {}; // Keep the serial watermark across resets.
        takeoff_release_at_=0; takeoff_native_={}; takeoff_jump_observed_=false;
        revert_link_={}; revert_trigger_owned_=false;
    }
    // Original native observations prove the physical handoff independently of
    // HUD/score labels. This does not report a scored Revert or select FS/BS.
    void observe_manual_transition(const InputNativeContext& frame) noexcept {
        auto& link=revert_link_;
        if(!link.direction || link.confirmed_at)return;
        if(!frame.required || !frame.valid || frame.generations.owner!=link.owner ||
            frame.sampled_at<link.started || frame.sampled_at-link.started>1500 ||
            frame.airborne || frame.rail_family || frame.flip.mode!=1 ||
            (frame.flip.stance&0x40u) || (frame.flip.state!=100 && frame.flip.state!=101)) {
            link.transition_step=0;return;
        }
        if(frame.physics_step<=link.step || frame.physics_step<=link.observed_step)return;
        if(link.observed_step && (frame.physics_step!=link.observed_step+1 ||
            frame.sampled_at<link.observed_at || frame.sampled_at-link.observed_at>150))link.transition_step=0;
        link.observed_step=frame.physics_step;link.observed_at=frame.sampled_at;
        if(frame.flip.state==101){link.transition_step=frame.physics_step;return;}
        if(link.transition_step && frame.physics_step>link.transition_step &&
            (frame.flip.stance&0x20u)!=link.stance)link.confirmed_at=frame.sampled_at;
    }
    // This is an admitted transition lease, not a revert success or score event.
    bool pending_manual_revert(std::uint64_t now,const InputNativeContext& frame) const noexcept {
        return revert_link_.direction && frame.required && frame.fresh(now) &&
            frame.generations.owner==revert_link_.owner && now>=revert_link_.started &&
            (revert_link_.confirmed_at ? now>=revert_link_.confirmed_at && now-revert_link_.confirmed_at<=1100 : now-revert_link_.started<=1500) && !frame.airborne && !frame.rail_family &&
            (frame.flip.state==100 || frame.flip.state==101) && frame.flip.mode<=1;
    }
    std::int32_t authored_air_followup(std::uint64_t now,const InputNativeContext& frame) const noexcept {
        return air_action_ && frame.required && frame.fresh(now) && frame.airborne &&
            frame.air_episode()==air_actor_episode_ && (native_followup_ ? native_flip::ordinary_flip(air_followup_id_) : ordinary_ollie_followup(air_followup_id_)) ? air_followup_id_ : 0;
    }
    void resume_manual_episode(const ManualInputDiagnostic& command,const InputNativeContext& native) noexcept {
        if(authored_.busy() || auxiliary_.busy() || rail_ || holding_ || board_active_)return;
        native_=native;native_required_=native.required;
        manual_.resume(command);
    }
    void cancel_manual() noexcept { manual_.cancel(); input_.reset_manual_continuity(); }
    bool initial_ground_acquisition(std::uint64_t now,const InputNativeContext& current) const noexcept {
        return native_.required && current.required && native_.fresh(now) && current.fresh(now) &&
            native_.generations.owner == current.generations.owner &&
            native_.generations.action == current.generations.action && manual_.initial_ground_acquisition(now);
    }
    // The facade calls this only while the existing rail adapter is inactive.
    // First-issue expiry retires input intent; it cannot time out an active
    // native method or turn a held physical control into another press.
    InputRequest expire_inactive_rail(std::uint64_t now) noexcept {
        if (!export_rail_ || !rail_ || rail_request_.action != InputAction::rail_request ||
            rail_request_.unbounded || now < rail_request_.recognized_at ||
            (rail_request_.inclusive_deadline ? now <= rail_request_.deadline : now < rail_request_.deadline)) return {};
        const auto expired = rail_request_;
        rail_ = false; rail_request_ = {};
        return expired;
    }
    void set_schedule(AuthoredPlaybackSchedule schedule) noexcept {
        if (authored_.schedule() == schedule) return;
        reset(); authored_.set_schedule(schedule);
    }
    void set_recognition_capture(bool enabled) noexcept {input_.set_recognition_capture(enabled);}
    bool busy() const noexcept { return auxiliary_.busy() || authored_.busy() || manual_.busy() || rail_ || board_active_ || extension_started_; }
    const UnifiedInput::Diagnostic& diagnostic() const noexcept { return input_.diagnostic(); }
    const InputHistory& history() const noexcept { return input_.history(); }
    InputRequest pending_manual() const noexcept { return input_.pending_manual(); }
    InputRequest pending_grab() const noexcept { return input_.pending_grab(); }
    NativeGesture::Output poll(std::uint64_t now, PhysicalInputSample physical,
                              std::int16_t rx, std::int16_t ry, InputAdmission admission) noexcept {
        NativeGesture::Output output{physical.buttons, rx, ry};
        // Recognition below still receives the original physical sample. A
        // gameplay-owned early return must not expose mapped face buttons.
        if (admission.gameplay && !admission.offboard && !(physical.buttons & 0x0030u))
            output.buttons &= static_cast<std::uint16_t>(~0xf00fu);
        const auto native = admission.native_context;
        native_required_ = native_required_ || native.required;
        if (native_required_ && (!native.required || !native.fresh(now) || !native.follows(native_))) {
            reset(); return output;
        }
        if (native.required) {
            if (!native_.valid || native.generations.owner != native_.generations.owner) reset();
            if (air_action_ && (!native.airborne || native.air_episode() != air_actor_episode_)) {
                authored_.cancel(); air_action_ = false; air_actor_episode_ = {};
            }
            if (grab_actor_ && (!native.airborne || native.air_episode() != grab_actor_episode_)) {
                auxiliary_.cancel(); grab_hold_button_ = 0; grab_actor_ = false; grab_actor_episode_ = {};
            }
            native_ = native;
            if (!admission.balance_controls.correlated(native))
                admission.balance_controls.admitted = false;
            if (!native.correlates_action(admission.balance_generations)) admission.manual_balance_admitted = false;
            if (!native.correlates_action(admission.rail.generations)) admission.rail.fresh = admission.rail.ready = false;
            if (!native.correlates_action(admission.surface.generations)) admission.surface.fresh = false;
            if (native.rail_family && !admission.offboard) {
                admission.pose = SkatePose::grind;
                admission.may_start = admission.may_push = admission.manual_safe = admission.manual_prepare = false;
                admission.manual_balance_admitted = false;
            }
        }
        // Only an already-started revert may retain ownership through its own
        // crouch/setup and native slide phases; this does not admit new requests.
        if (auxiliary_.busy() && !grab_actor_ && native.required && native.fresh(now) &&
            revert_context(admission,now))
            admission.action_fresh=true;
        if (clocked_ && (admission.input_policy != policy_ || admission.board_binding != board_binding_ || admission.offboard != offboard_ || admission.export_rail_requests != export_rail_)) reset();
        offboard_ = admission.offboard; board_binding_ = admission.board_binding; export_rail_ = admission.export_rail_requests;
        policy_ = admission.input_policy;
        if (clocked_ && admission.action_mode != mode_) {
            auxiliary_.cancel(); grab_hold_button_ = 0; input_.cancel_auxiliary(); revert_link_={};
        }
        mode_ = admission.action_mode;
        if (!admission.gameplay || (physical.buttons & 0x0030u) || admission.pose == SkatePose::bail ||
            (!admission.offboard && admission.pose == SkatePose::unknown && !manual_.busy()) ||
            (clocked_ && (now < last_ || admission.tick < last_tick_))) {
            reset();
            if (admission.gameplay && admission.offboard) output.right_x = output.right_y = 0;
            return output;
        }
        clocked_ = true; last_ = now; last_tick_ = admission.tick;
        now_ = now; physical_ = physical; admission_ = admission; current_ = &output; extension_started_ = false;
        if (takeoff_release_at_) {
            if (admission.external_owner || (physical.buttons & 0xa000u) || physical.lt>30 || physical.rt>30 || now<takeoff_release_at_ || (!native.airborne && now-takeoff_release_at_>=InputTiming::direction_ms) ||
                !native.released_jump_continues(takeoff_native_,now)) takeoff_release_at_=0;
            else {takeoff_native_=native;takeoff_jump_observed_=takeoff_jump_observed_ || native.flip.id==1;}
        }
        if (!admission.offboard) output.buttons &= static_cast<std::uint16_t>(~0xf00fu);
        output.right_x = output.right_y = 0; // Physical right stick belongs to the camera.
        authored_.finish_neutral_tail();
        if (air_action_ && admission.pose != SkatePose::air) { authored_.cancel(); air_action_ = false; }
        if (admission.external_owner || auxiliary_.busy()) { authored_.cancel(); holding_ = false; }
        // Shared intent was withdrawn by the competing owner. Do not retain a
        // held rail ticket that can resume output or block later fresh actions.
        // The journal still observes the physical baseline through this gap.
        if (admission.external_owner) { rail_ = false; rail_request_ = {}; }
        const bool rail_released = rail_ && !(physical.buttons & 0x8000u);
        if (rail_released) rail_request_.physical_released_at = now;
        // A performed-Lip observation retires the generic ticket. It does not
        // fabricate a physical Y release or reset the held-event baseline.
        if (!(physical.buttons & 0x8000u) || admission.observed_lip) rail_ = false;
        if(revert_link_.direction) {
            const bool expired=now<revert_link_.started || (!revert_link_.confirmed_at && now-revert_link_.started>1500) ||
                (revert_link_.confirmed_at && (now<revert_link_.confirmed_at || now-revert_link_.confirmed_at>1100));
            if(expired || !native.required || !native.fresh(now) || native.generations.owner!=revert_link_.owner ||
                native.airborne || native.rail_family || admission.external_owner || physical.lt>30 ||
                (physical.buttons & 0x9330u)) revert_link_={};
            else if(revert_link_.issued && !auxiliary_.busy() && admission.manual_balance_admitted && manual_.established_balance() &&
                manual_.requested()==revert_link_.direction &&
                admission.pose==(revert_link_.direction<0?SkatePose::manual:SkatePose::nose_manual)) revert_link_={};
            else if(revert_link_.issued && !manual_.busy()) revert_link_={};
            else if(!revert_link_.issued && revert_link_.confirmed_at && !manual_.busy() && admission.manual_safe &&
                native.flip.state==100 && native.flip.mode==1 &&
                (native.flip.stance&0x20u)!=revert_link_.stance) {
                // Native acknowledgment and changed stance transfer only RS.
                // The existing revert keeps its complete LS trajectory. Waiting
                // for that actor to finish would keep pressing the old manual
                // through the opposite contact, then emit an unowned neutral poll.
                if(manual_.request(now,revert_link_.direction,false,true,false,
                    revert_link_.started,revert_link_.recognition)) revert_link_.issued=true;
            }
        }
        const bool manual_revert_handoff=revert_link_.issued && !grab_actor_;
        if (physical.rt<=30) revert_trigger_owned_=false;
        else if(auxiliary_.busy() && !grab_actor_) revert_trigger_owned_=true;
        const bool triggers = physical.lt > 30 || physical.rt > 30;
        // Native adapter wiring only: an admitted balance can retain its RS
        // hold when a stance/branch trigger has no competing channel owner.
        // Preparation and actual authored/auxiliary/external conflicts still cancel.
        const bool manual_trigger_only=triggers && !authored_.busy() && !auxiliary_.busy() && !admission.external_owner;
        // Unavailable ground flip/grab buttons have no competing actuator.
        // Keep their physical edges in recognition, but do not release a valid
        // manual hold merely because the old native face-button mask included them.
        const bool ground_balance = admission.pose == SkatePose::manual || admission.pose == SkatePose::nose_manual ||
            (revert_link_.issued && native.required && native.fresh(now) && !native.airborne &&
             native.flip.state==100 && native.flip.mode==1);
        const auto manual_buttons = static_cast<std::uint16_t>(physical.buttons & (ground_balance ? ~0x6000u : 0xffffu));
        // During the acknowledged handoff the client still publishes the old
        // episode type. The new actuator waits for successor admission, not
        // that old presentation type (which would cancel it as opposite input).
        const auto actuator_pose = revert_link_.issued && native.required && native.fresh(now) &&
            native.flip.state==100 && native.flip.mode==1 ?
            (admission.pose==(revert_link_.direction<0?SkatePose::manual:SkatePose::nose_manual) ?
                (revert_link_.direction<0?SkatePose::manual:SkatePose::nose_manual) : SkatePose::riding) : admission.pose;
        const bool manual_was_active = manual_.busy();
        auto manual = manual_.sample(now, true, admission.manual_safe, actuator_pose,
            authored_.busy() || (auxiliary_.busy() && !manual_revert_handoff) || admission.external_owner ||
            (triggers && !manual_revert_handoff), manual_buttons, admission.manual_balance_admitted,manual_trigger_only,admission.manual_contact_only,held_air_manual());
        const bool reset_manual = manual_was_active && !manual_.busy() && manual_.hard_reset();
        const bool manual_contact = admission.pose == SkatePose::manual || admission.pose == SkatePose::nose_manual;
        const bool awaiting_admission = manual_contact && !admission.manual_balance_admitted && !manual_.busy() &&
            input_.pending_manual().action != InputAction::none;
        const InputContext context = admission.offboard ? InputContext::foot : admission.observed_lip ? InputContext::lip : admission.pose == SkatePose::air ? InputContext::air :
            manual_contact && !awaiting_admission ? InputContext::balance :
            admission.pose == SkatePose::grind ? InputContext::rail : InputContext::ground;
        const bool action_enabled = !admission.offboard && admission.action_mode != ActionExperiment::off;
        if (!admission.action_fresh || !action_enabled) auxiliary_.cancel();
        // Retire prior auxiliary state before admitting this poll's commands.
        // This output is not delivered; the normal sample below still applies
        // all current input ownership and native context guards.
        const ActionContext prior_action_context{admission.action_mode != ActionExperiment::observe,
            admission.action_fresh, context == InputContext::air, context == InputContext::ground ||
            (auxiliary_.busy() && !grab_actor_ && revert_context(admission,now)),
            admission.inspected_stance, admission.external_owner, verified_manual(admission,now)};
        (void) auxiliary_.sample(now,prior_action_context);
        output.action.consume_buttons = action_enabled && admission.action_fresh &&
            (admission.action_mode == ActionExperiment::grabs ||
             (admission.action_mode == ActionExperiment::classic && context == InputContext::air)) ? 0x2000 : 0;
        output.action.consume_rt = action_enabled && ((revert_trigger_owned_ && revert_context(admission,now)) || admission.action_fresh) &&
            (admission.action_mode == ActionExperiment::classic || admission.action_mode == ActionExperiment::revert_frontside ||
             admission.action_mode == ActionExperiment::revert_backside);
        const bool manual_conflict = authored_.busy() || auxiliary_.busy() || admission.external_owner || triggers;
        // Ground auxiliary input is RT; its table uses the same experiment
        // permission as physical RT masking, without enabling a hidden actor.
        InputEnvironment environment{context, true, admission.external_owner, admission.nollie, admission.observed_balance_branch ? admission.observed_balance_branch : admission.pose == SkatePose::nose_manual ? 13u : 0u,
            admission.manual_safe && !admission.external_owner && !triggers,
            admission.manual_prepare && !admission.external_owner && !triggers, manual_.busy() || reset_manual,
            action_enabled && admission.action_fresh && (context != InputContext::ground || output.action.consume_rt),
            admission.may_start && !admission.offboard,
            admission.pose == SkatePose::riding || admission.pose == SkatePose::air || manual_.busy() || awaiting_admission,
            admission.action_mode == ActionExperiment::grabs, admission.input_policy, admission.branch_ready && admission.manual_balance_admitted, admission.rail, admission.surface,
            admission.named_grab_bindings || admission.action_mode == ActionExperiment::grabs, admission.balance_controls,
            admission.may_toggle_board && !board_active_, admission.board_binding, native};
        environment.native_pop_transition=admission.normal_flip.submit!=nullptr;
        environment.takeoff_release_at=takeoff_jump_observed_ ? takeoff_release_at_ : 0;
        // A released owned setup with an independently selected native Ollie
        // may SELECT X in this poll; the actual release below still delivers
        // the pop and execution still requires genuine AIR and delivered arcs.
        if (holding_ && !(physical.buttons&classic_a) && !jump_nollie_ &&
            launch_context_==GestureContext::ground && native.required && native.fresh(now) &&
            !native.airborne && native.flip.valid && native.flip.id==1 && native.flip.object &&
            (native.flip.state==100 || native.flip.state==103)) environment.takeoff_release_at=now;
        environment.takeoff_actor_busy=authored_.busy() || auxiliary_.busy();
        input_.poll(now, physical, environment, {this, &receive});
        if (reset_manual) input_.reset_manual_continuity();
        // A released A ends native setup even when another request superseded
        // recognition in this poll. No new edge or jump is synthesized here.
        if (holding_ && !(physical.buttons & classic_a)) {
            output.jump = {true,true,jump_nollie_,launch_context_ == GestureContext::from_grind,jump_pressed_at_,now,jump_launch_};
            authored_.release(admission.tick); holding_ = false; input_.acknowledge_jump_release();
        }
        if (output.jump.released && output.jump.setup_admitted && !output.jump.from_rail &&
            native.required && native.fresh(now) && native.flip.valid && (native.flip.id==1 || native.flip.id==-1) && native.flip.object &&
            !native.airborne && (native.flip.state==100 || native.flip.state==103)) {
            takeoff_release_at_=output.jump.released_at; takeoff_native_=native;takeoff_jump_observed_=native.flip.id==1;
        }
        const auto authored = authored_.sample(admission.tick, now);
        if (authored.owns_stick) {
            output.right_x = authored.x; output.right_y = authored.y; output.phase = 1;
            output.authored_playback = true;
        }
        manual = manual_.sample(now, true, admission.manual_safe, actuator_pose,
            authored_.busy() || (auxiliary_.busy() && !manual_revert_handoff) || admission.external_owner ||
            (triggers && !manual_revert_handoff), manual_buttons, admission.manual_balance_admitted,manual_trigger_only,admission.manual_contact_only,held_air_manual());
        if (manual.owns_stick) { output.right_x = 0; output.right_y = manual.y; output.phase = 1; }
        const auto& diagnostic = input_.diagnostic();
        output.suppress_reverse = !manual_.busy() && diagnostic.suppress_reverse;
        // During AIR preparation/first landing the held final direction is a
        // command modifier owned by the delivered RS actor. Do not also send
        // native LY (Down selects the ordinary ground reverse/revert path).
        // Actual confirmed balance retains its existing correction writer.
        if (manual.owns_stick && manual_.from_air() && (!manual.confirmed || !admission.manual_balance_admitted) &&
            InputEvents::normalize(physical,now,true,admission.input_policy).direction==(manual_.requested()<0?2:8))
            output.suppress_reverse=true;
        output.manual = {diagnostic.stick, diagnostic.pad,
            diagnostic.stick_first, diagnostic.pad_first, manual_.requested(), diagnostic.stick_at,
            diagnostic.pad_at, manual_.requested_at(),
            !admission.manual_safe && !admission.manual_prepare ? 4u : manual_conflict ? 2u : 0u,
            admission.manual_safe, admission.manual_prepare, manual.owns_stick, manual.confirmed,
            manual_.recognized_at(), manual_.from_air(),manual_.recognition_id(),manual_.held_air()};
        if(revert_link_.issued && manual.owns_stick && manual_.requested()==revert_link_.direction)
            output.manual.revert={revert_link_.owner,revert_link_.step,revert_link_.contact,
                revert_link_.confirmed_at,revert_link_.stance};
        bool busy = authored_.busy() || manual_.busy() || input_.pending_stick() || extension_started_;
        if (rail_ && !export_rail_ && !busy && !triggers && !auxiliary_.busy() &&
            (context == InputContext::air || context == InputContext::rail)) {
            output.right_x = 0;
            output.right_y = rail_direction_ == TrickDirection::up ? 20000 : rail_direction_ == TrickDirection::down ? -20000 : 0;
            output.phase = 1; busy = true;
        }
        ActionContext action_context{admission.action_mode != ActionExperiment::observe, admission.action_fresh,
            context == InputContext::air, context == InputContext::ground ||
            (auxiliary_.busy() && !grab_actor_ && revert_context(admission,now)), admission.inspected_stance,
            authored_.busy() || input_.pending_stick() || extension_started_ ||
            (manual_.busy() && !manual_revert_handoff) || board_active_ || admission.external_owner || (physical.buttons & (0x5000u | 0x0300u | 0x0010u)) != 0,
            verified_manual(admission,now)};
        if (grab_hold_button_ && !(physical.buttons & grab_hold_button_)) { auxiliary_.release_grab(now); grab_hold_button_ = 0; }
        if ((grab_actor_ ? !admission.inspected_stance : !revert_context(admission,now)) ||
            (admission.action_mode == ActionExperiment::classic && admission.nollie)) auxiliary_.cancel();
        output.action.pad = auxiliary_.sample(now, action_context);
        if(manual_revert_handoff && manual.owns_stick)output.action.pad.owns_right_stick=false;
        if (!auxiliary_.busy()) { grab_actor_ = false; grab_actor_episode_ = {}; }
        const auto& action = output.action.pad;
        output.action.owned = action.owns_right_stick || action.owns_left_stick || action.owns_triggers || action.owns_face_buttons;
        const bool board_permitted = admission.may_toggle_board && !busy && !output.action.owned && !triggers &&
            !admission.external_owner && !(physical.buttons & static_cast<std::uint16_t>(~board_control_));
        if (!board_permitted || (board_active_ && now - board_at_ >= BoardChord::pulse_ms)) board_active_ = false;
        const auto board = board_.poll(now, board_active_ ? BoardChord::pair : 0, true, board_permitted);
        if (!(physical.buttons & board_consumed_)) board_consumed_ = 0;
        output.buttons &= static_cast<std::uint16_t>(~board_consumed_);
        if (board.pulse) {
            output.buttons = static_cast<std::uint16_t>((output.buttons & ~board_control_) | BoardChord::native_y);
            output.phase = 3; busy = true;
        }
        const bool conflict = (physical.buttons & ~classic_x) || triggers || busy || output.action.owned || admission.external_owner;
        output.automatic_push = push_.poll(now, physical.x, physical.y, admission.speed, true,
            !admission.offboard && admission.pose == SkatePose::riding && admission.may_push && !conflict);
        if (output.automatic_push) output.buttons |= NativeGesture::x;
        output.movement_intent = push_.movement_intent();
        if (export_rail_ && (rail_ || rail_released)) {
            output.rail = {rail_request_,rail_,busy || triggers || auxiliary_.busy() || board.pulse || admission.external_owner,output.action.pad};
            output.rail.occupied.owns_right_stick = output.rail.occupied.owns_right_stick || busy || admission.external_owner;
        }
        if (context != InputContext::ground || admission.pose != SkatePose::riding || admission.offboard ||
            admission.observed_lip || busy || output.action.owned || board.pulse || board_active_ ||
            admission.external_owner || triggers || holding_ || (physical.buttons & classic_a) || !native.required || !native.fresh(now) ||
            !native.correlates_action(ground_turn_.generations)) ground_turn_ = {};
        output.ground_turn = ground_turn_;
        output.command_busy = busy || output.action.owned || board.pulse || (export_rail_ && rail_);
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
    bool native_followup_{};
    InputDisposition request(const InputRequest& request) noexcept {
        if (!current_) return InputDisposition::cancelled;
        auto& out = *current_;
        const auto& admission = admission_;
        const bool airborne = admission.pose == SkatePose::air;
        const auto context = admission.pose == SkatePose::grind ? GestureContext::from_grind : launch_context_;
        switch (request.action) {
        case InputAction::board_toggle:
            if (!admission.may_toggle_board || authored_.busy() || manual_.busy() || auxiliary_.busy() ||
                admission.external_owner || physical_.lt > 30 || physical_.rt > 30 ||
                (physical_.buttons & static_cast<std::uint16_t>(~request.control))) return InputDisposition::cancelled;
            board_.reset(); board_.poll(now_,0,true,true); board_active_ = true;
            board_at_ = now_; board_control_ = board_consumed_ = request.control; return InputDisposition::started;
        case InputAction::crouch:
            // Lip exits require their own admitted actor and release policy.
            if (admission.observed_lip) { out.unsupported = true; return InputDisposition::unsupported; }
            if (!admission.launch.ordinary_writer()) { out.unsupported = true; return InputDisposition::unsupported; }
            if (airborne || !admission.may_start || authored_.busy() || auxiliary_.busy() || manual_.busy()) return InputDisposition::cancelled;
            launch_context_ = admission.pose == SkatePose::grind ? GestureContext::from_grind : GestureContext::ground;
            holding_ = start(request.nollie ? 16 : 1, request.nollie, true, launch_context_, false);
            if (holding_) {
                jump_pressed_at_ = request.physical_pressed_at; jump_nollie_ = request.nollie; jump_launch_ = admission.launch;
                if(launch_context_==GestureContext::from_grind)jump_launch_.context=LaunchContext::rail;
                out.jump={false,false,jump_nollie_,launch_context_==GestureContext::from_grind,jump_pressed_at_,0,jump_launch_,true};
            }
            return holding_ ? InputDisposition::started : InputDisposition::unsupported;
        case InputAction::ollie_release:
            out.jump = {true,holding_,request.nollie,launch_context_ == GestureContext::from_grind,request.physical_pressed_at,request.physical_released_at,jump_launch_};
            if (holding_) authored_.release(admission.tick);
            holding_ = false; return InputDisposition::started;
        case InputAction::flip: {
            if (!airborne) return request.pop_release_at ? InputDisposition::waiting : InputDisposition::cancelled;
            if (admission.native_context.required && (!admission.native_context.airborne || !admission.native_context.airborne_episode)) {
                out.unsupported = true; return InputDisposition::unsupported;
            }
            // A released, independently observed ordinary Ollie may hand its
            // completed RS flick to another native gesture during ascent. The
            // selection ID describes the current trick, not a pad-input lock.
            // Keep generic/late requests on their existing idle readiness gate.
            const bool own_ollie = takeoff_release_at_ && takeoff_jump_observed_ &&
                admission.action_fresh && admission.native_context.fresh(now_) &&
                admission.native_context.airborne && admission.native_context.flip.valid &&
                (admission.native_context.flip.id == 1 || admission.native_context.flip.id == -1);
            // Recognition already selected this request from the physical X
            // edge. Retire only this released pop's delivered actuator, never
            // a held setup or an airborne flip. Native AIR/Ollie continuity is
            // independent proof; the launch ticket and trajectory are untouched.
            if (own_ollie && !air_action_ && !auxiliary_.busy())
                authored_.handoff_delivered_pop(admission.tick,now_);
            if ((!admission.flip_ready && !own_ollie) || authored_.busy() || auxiliary_.busy()) return InputDisposition::waiting;
            if (request.form != InputForm::ordinary && request.form != InputForm::neutral_release && request.form != InputForm::neutral_press) return request_extension(request);
            {
                if ((request.form == InputForm::neutral_release || request.form == InputForm::neutral_press) &&
                    (request.direction || request.presses != 1 || request.air.family != TrickFamily::flip ||
                     request.air.trick != ArcadeTrick::kickflip || request.air.predicate != AirInputPredicate::face)) {
                    out.unsupported = true; return InputDisposition::unsupported;
                }
                const auto selected=(request.form == InputForm::neutral_release || request.form == InputForm::neutral_press) ? ArcadeTrick::kickflip :
                    select_named_trick(TrickFamily::flip,direction(request.direction),request.presses);
                if(request.air.trick!=ArcadeTrick::none && (request.air.family!=TrickFamily::flip || request.air.trick!=selected)){
                    out.unsupported=true;return InputDisposition::unsupported;
                }
                const auto id = native_flip_id(selected,request.nollie);
                if (own_ollie && admission.normal_flip.submit) {
                    if (!admission.normal_flip.submit(admission.normal_flip.context,id)) return InputDisposition::waiting;
                    out.native_flip_request=id;native_followup_=true;
                    air_action_=true;air_actor_episode_=admission.native_context.air_episode();air_followup_id_=id;
                    return InputDisposition::started;
                }
                native_followup_=false;
                air_action_ = start(id, request.nollie, false, context, true);
                if (air_action_) { air_actor_episode_ = admission.native_context.air_episode(); air_followup_id_=id; }
                return air_action_ ? InputDisposition::started : InputDisposition::unsupported;
            }
        }
        case InputAction::manual: case InputAction::nose_manual:
            if (admission.external_owner || physical_.lt > 30 || physical_.rt > 30 ||
                (physical_.buttons & 0xf030u) || (physical_.buttons & 0x0300u) == 0x0300u) return InputDisposition::cancelled;
            if (request.context == InputContext::air && now_ - request.recognized_at > InputTiming::air_request_ms)
                return InputDisposition::expired; // Queue and native adapter share the inclusive preparation deadline.
            if (!admission.manual_safe) return admission.manual_prepare ||
                (admission.pose == SkatePose::riding && admission.native_context.manual_ground_wait(now_)) ?
                InputDisposition::waiting : InputDisposition::cancelled;
            if (authored_.busy() || auxiliary_.busy()) return InputDisposition::waiting;
            manual_air_episode_ = admission.native_context.air_episode();
            return manual_.request(now_, request.action == InputAction::manual ? -1 : 1, airborne, admission.manual_safe,
                request.context == InputContext::air, request.recognized_at,
                request.recognition ? request.recognition->sequence : 0,
                held_air_manual(request.action == InputAction::manual ? -1 : 1))
                ? InputDisposition::started : InputDisposition::cancelled;
        case InputAction::rail_request:
            if (admission.observed_lip) { out.unsupported = true; return InputDisposition::unsupported; }
            if (!(physical_.buttons & 0x8000u)) return InputDisposition::cancelled;
            rail_direction_ = direction(request.direction);
            if (rail_direction_ != TrickDirection::none && rail_direction_ != TrickDirection::up && rail_direction_ != TrickDirection::down) {
                out.unsupported = true; return InputDisposition::unsupported;
            }
            rail_ = true; rail_request_ = request; return InputDisposition::started;
        case InputAction::grab: case InputAction::revert:
            return request_auxiliary(request);
        case InputAction::rail_select: case InputAction::rail_change: case InputAction::lip_select: case InputAction::lip_change:
            if (!admission.rail.fresh) return request.action == InputAction::rail_select &&
                (admission.pose == SkatePose::riding || admission.pose == SkatePose::air) ? InputDisposition::waiting : InputDisposition::cancelled;
            if ((request.action != InputAction::rail_select || request.rail_generation != 0) &&
                request.rail_generation != admission.rail.generation) return InputDisposition::cancelled;
            if (request.action == InputAction::lip_select && admission.rail.phase == InputRailPhase::lip_selection) {
                if (admission.rail.gameframe < admission.rail.selection_frame) return InputDisposition::cancelled;
                if (admission.rail.gameframe - admission.rail.selection_frame < 2) return InputDisposition::waiting;
            }
            if (!admission.rail.ready || !admission.rail.geometry_known) return InputDisposition::waiting;
            return request_extension(request);
        case InputAction::wall_command: case InputAction::transfer_command:
            if (!admission.surface.fresh || request.surface_generation != admission.surface.generation) return InputDisposition::cancelled;
            if (request.action == InputAction::wall_command && !(request.variant == 1 ? admission.surface.can_plant : admission.surface.can_push)) return InputDisposition::cancelled;
            if (request.action == InputAction::transfer_command && !admission.surface.can_transfer) return InputDisposition::cancelled;
            return request_extension(request);
        case InputAction::balance_branch_one: case InputAction::balance_branch_two: case InputAction::balance_variant: case InputAction::balance_return: {
            const auto origin = admission.observed_balance_branch ? admission.observed_balance_branch :
                admission.pose == SkatePose::nose_manual ? 13u : 0u;
            if (!admission.manual_balance_admitted || !admission.branch_ready || origin != request.balance_origin)
                return InputDisposition::cancelled;
            return request_extension(request);
        }
        case InputAction::lip_request:
            return request_extension(request);
        case InputAction::ground_turn: {
            if (request.variant != 1 || request.context != InputContext::ground ||
                admission.pose != SkatePose::riding || admission.offboard || admission.observed_lip ||
                admission.external_owner || authored_.busy() || manual_.busy() || auxiliary_.busy() || board_active_ ||
                holding_ || (physical_.buttons & classic_a) || physical_.lt > 30 || physical_.rt > 30) return InputDisposition::cancelled;
            const auto& native=admission.native_context;
            if (!native.required || !native.fresh(now_) || native.airborne || native.rail_family ||
                now_ < request.recognized_at || now_-request.recognized_at > 150 ||
                !native.correlates_action(request.generations) || ground_turn_serial_ == UINT64_MAX) {
                out.unsupported = true;
                return InputDisposition::unsupported;
            }
            if (ground_turn_.serial && now_ >= ground_turn_.requested_at && now_-ground_turn_.requested_at < 300)
                return InputDisposition::cancelled;
            ground_turn_={++ground_turn_serial_,request.recognized_at,native.generations,native.physics_step,native.sampled_at};
            return InputDisposition::started;
        }
        default: return InputDisposition::unsupported;
        }
    }
    InputDisposition request_extension(const InputRequest& request) noexcept {
        auto result = InputDisposition::unsupported;
        if (admission_.extensions.request) result = admission_.extensions.request(admission_.extensions.state,request);
        extension_started_ = extension_started_ || result == InputDisposition::started;
        current_->unsupported = result == InputDisposition::unsupported;
        return result;
    }
    static bool verified_manual(const InputAdmission& a,std::uint64_t now) noexcept {
        return a.manual_balance_admitted && a.branch_ready && a.native_context.required &&
            a.native_context.fresh(now) && a.native_context.correlates_action(a.balance_generations) &&
            a.native_context.flip.state==100 && a.native_context.flip.mode==1 &&
            (a.pose==SkatePose::manual || a.pose==SkatePose::nose_manual);
    }
    static bool revert_context(const InputAdmission& admission,std::uint64_t now) noexcept {
        const auto& n=admission.native_context;
        if(!n.required)return admission.inspected_stance;
        return n.fresh(now) && !n.airborne && !n.rail_family &&
            (n.flip.state==100 || n.flip.state==101 || n.flip.state==103) && n.flip.mode<=1;
    }
    InputDisposition request_auxiliary(const InputRequest& request) noexcept {
        auto& out = current_->action;
        const auto& admission = admission_;
        const bool grab = request.action == InputAction::grab;
        const bool classic = admission.action_mode == ActionExperiment::classic;
        if (!admission.action_fresh || admission.action_mode == ActionExperiment::off) return InputDisposition::cancelled;
        if (authored_.busy() || (manual_.busy() && (grab || !verified_manual(admission,now_))) || auxiliary_.busy() || rail_) return InputDisposition::waiting;
        if (request.form != InputForm::ordinary) return request_extension(request);
        out.requested = true; out.request_kind = grab ? 1u : admission.action_mode == ActionExperiment::revert_backside ? 4u : 3u;
        if (grab && admission.native_context.required && (!admission.native_context.airborne || !admission.native_context.airborne_episode)) {
            current_->unsupported = true; out.result = DispatchResult::unavailable; return InputDisposition::unsupported;
        }
        ActionContext context{admission.action_mode != ActionExperiment::observe, true,
            admission.pose == SkatePose::air, admission.pose == SkatePose::riding, admission.inspected_stance,
            admission.external_owner || (physical_.buttons & (0x5000u | 0x0300u | 0x0010u)) != 0,
            verified_manual(admission,now_)};
        if (grab) {
            out.trick = classic && !request.named_binding && request.presses == 1 ? ArcadeTrick::basic_bs_grab :
                select_named_trick(TrickFamily::grab, direction(request.direction), request.presses);
            if (out.trick == ArcadeTrick::basic_bs_grab && request.control && !(physical_.buttons & request.control)) {
                // Recognition survives release. The native adapter must supply its
                // minimum initialization/out stage before a released tap can run.
                out.result = DispatchResult::unavailable; return InputDisposition::waiting;
            }
            out.result = classic && (out.trick != ArcadeTrick::basic_bs_grab || request.nollie || admission.nollie) ? DispatchResult::unavailable :
                admission.action_mode == ActionExperiment::grabs || classic ? auxiliary_.request_grab(out.trick, now_, context, out.trick == ArcadeTrick::basic_bs_grab) : DispatchResult::disabled;
            if (out.result == DispatchResult::started && out.trick == ArcadeTrick::basic_bs_grab) grab_hold_button_ = request.control;
            if (out.result == DispatchResult::started) {
                grab_actor_ = true; grab_actor_episode_ = admission.native_context.air_episode();
            }
        } else {
            const bool backside = admission.action_mode == ActionExperiment::revert_backside ||
                (classic && (request.direction==1 || request.direction==4 || request.direction==7));
            out.request_kind=backside?4u:3u;
            const bool native_start = !admission.native_context.required ||
                (admission.native_context.flip.state==100 && admission.native_context.flip.mode<=1);
            out.result = !native_start || !revert_context(admission,now_) || (classic && admission.nollie) ? DispatchResult::unsafe_context :
                auxiliary_.request_revert(backside ? RevertSide::backside : RevertSide::frontside,
                    {context.verified_manual ? admission.pose==SkatePose::nose_manual : admission.nollie,
                     admission.mirror_x, false, static_cast<std::int16_t>(context.verified_manual ? (admission.pose==SkatePose::manual?-20000:20000) : 0)}, now_, context);
            if(out.result==DispatchResult::started) revert_trigger_owned_=physical_.rt>30;
            if(out.result==DispatchResult::started && context.verified_manual) {
                revert_link_={admission.pose==SkatePose::manual?1:-1,now_,0,
                    admission.native_context.generations.owner,admission.native_context.flip.stance&0x20u,
                    request.recognition?request.recognition->sequence:0,
                    admission.native_context.physics_step,admission.native_context.generations.contact,false};
                manual_.cancel(); input_.reset_manual_continuity();
            }
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
    bool held_air_manual(int direction=0) const noexcept {
        if (!direction) direction=manual_.requested();
        const auto& native=admission_.native_context;
        if (!direction || !native.required || !native.fresh(now_) || !manual_air_episode_.number ||
            native.airborne_episode!=manual_air_episode_.number) return false;
        const bool same_flight=native.airborne && native.air_episode()==manual_air_episode_;
        const bool first_landing=!native.airborne && native.flip.state==100 && native.flip.mode==1 &&
            native.air_departure>manual_air_episode_.departure && native.air_departure-manual_air_episode_.departure==1;
        const auto physical=InputEvents::normalize(physical_,now_,true,admission_.input_policy);
        return (same_flight || first_landing) && physical.direction==(direction<0?2:8);
    }
    struct RevertLink {int direction{};std::uint64_t started{},confirmed_at{},owner{};
        std::uint32_t stance{};std::uint64_t recognition{},step{},contact{};bool issued{};
        std::uint64_t transition_step{},observed_step{},observed_at{};} revert_link_;
    bool revert_trigger_owned_{};
    UnifiedInput input_{};
    AuthoredPlayback authored_{};
    ManualActuation manual_{};
    ActionDispatch auxiliary_{};
    AutomaticPush push_{};
    BoardChord board_{};
    bool board_active_{}, offboard_{}, export_rail_{};
    InputRequest rail_request_{};
    std::uint64_t board_at_{};
    std::uint16_t board_control_{BoardChord::pair}, board_consumed_{};
    InputBoardBinding board_binding_{};
    ActionExperiment mode_{ActionExperiment::off};
    InputPolicy policy_{InputPolicy::production()};
    InputNativeContext native_{};
    InputNativeContext takeoff_native_{};
    std::uint64_t takeoff_release_at_{};
    bool takeoff_jump_observed_{};
    InputAirEpisode air_actor_episode_{}, grab_actor_episode_{}, manual_air_episode_{};
    bool grab_actor_{};
    bool native_required_{};
    InputAdmission admission_{};
    PhysicalInputSample physical_{};
    NativeGesture::Output* current_{};
    GestureContext launch_context_{GestureContext::ground};
    LaunchDescriptor jump_launch_{};
    GroundTurnCommand ground_turn_{};
    std::uint64_t ground_turn_serial_{};
    TrickDirection rail_direction_{};
    std::uint64_t now_{}, last_{}, last_tick_{}, jump_pressed_at_{};
    bool jump_nollie_{}, extension_started_{};
    std::int32_t air_followup_id_{};
    bool rail_{}, holding_{}, air_action_{}, clocked_{};
    std::uint16_t grab_hold_button_{};
};
}
