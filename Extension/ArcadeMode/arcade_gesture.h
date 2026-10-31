#pragma once
#include "arcade_input_session.h"
#include "arcade_balance_input.h"
#include "arcade_native_input_state.h"
#include "arcade_rail_adapter.h"
#include "arcade_rail_adapter_probe.h"
#include "Engine/Game/Build/20260929/arcade_native_probe.h"
#include <array>
#include <cstdint>

namespace dingosdk::arcade_mode {
// Distinguish button masking, stick setup and flick from the native packet.
// Restoration must also be visible when the physical packet has not changed.
constexpr std::uint32_t synthetic_packet_mask(unsigned phase, bool masked_buttons) noexcept {
    return phase == 3 ? 0x10000000u : phase == 1 ? 0x40000000u : phase == 2 ? 0x20000000u : masked_buttons ? 0x60000000u : 0u;
}
constexpr std::uint32_t manual_reverse_packet_mask(bool owned) noexcept {
    return owned ? 0x08000000u : 0u;
}
// Public controller boundary. Each enabled slot has exactly one input session.
class ArcadeInput {
public:
    struct Physical { bool valid{}; std::uint64_t at{}; std::uint16_t buttons{}; short lx{},ly{},rx{},ry{}; std::uint8_t lt{},rt{}; };
    explicit ArcadeInput(bool native_bound = false, InputPolicy policy = InputPolicy::production()) noexcept
        : native_bound_(native_bound), policy_(policy) {}
    void observe_physical(unsigned user, Physical pad) noexcept {
        if(user==0){physical_=pad;physical_frame_=pad.valid ? native_ : NativeInputFrame{};}
    }
    Physical physical(std::uint64_t now) const noexcept {
        return physical_.valid && now >= physical_.at && now-physical_.at <= 150 ? physical_ : Physical{};
    }
    InputDigitalState digital_state(std::uint64_t now) const noexcept {
        const auto current=native_context(now);const auto& origin=physical_frame_;
        if(!physical_.valid || now<physical_.at || now-physical_.at>150 || !current.valid || !origin.valid ||
            !origin.owner.valid() || origin.owner!=current.owner || origin.owner_generation!=current.owner_generation ||
            !origin.contact_generation || !origin.action_generation || !origin.physics_step || origin.physics_step>current.physics_step ||
            origin.sampled_at>current.sampled_at || origin.sampled_at>physical_.at ||
            origin.contact_generation>current.contact_generation || origin.action_generation>current.action_generation)return {};
        // Physical held state can span a new contact/action observation. Its
        // original owner/sample survives; it never supplies action readiness.
        return InputEvents::normalize({physical_.buttons,physical_.lx,physical_.ly,physical_.lt,physical_.rt},physical_.at,true,policy_);
    }
    InputControlPair configured_controls(std::uint64_t now,InputBalanceControls controls,const NativeInputFrame& proof) const noexcept {
        const auto current=native_context(now);
        if(!controls.admitted || !current.valid || !proof.valid || !proof.owner.valid() || proof.owner!=current.owner ||
            !proof.owner_generation || !proof.contact_generation || !proof.action_generation || !proof.physics_step ||
            proof.owner_generation!=current.owner_generation || proof.contact_generation!=current.contact_generation ||
            proof.action_generation!=current.action_generation || proof.physics_step>current.physics_step ||
            proof.sampled_at>current.sampled_at || now<proof.sampled_at || now-proof.sampled_at>150)return {};
        return digital_state(now).configured(controls);
    }
    InputControlPair configured_balance(std::uint64_t now) const noexcept {
        const auto current=native_context(now);
        if(!balance_controls_current(now,current) || balance_frame_.action_generation!=current.action_generation)return {};
        return configured_controls(now,balance_controls_,balance_frame_);
    }
    bool publish_authority(std::uint64_t now, std::uint64_t owner, unsigned source, bool focused) noexcept {
        focused = focused && owner && source < 0x100 && (source & 1u);
        const bool changed = owner_ != owner || source_ != source || focused_ != focused;
        if (changed) { cancel_commands(); balance_controls_ = {}; balance_frame_ = {}; }
        owner_ = owner; source_ = source; focused_ = focused; authority_at_ = now;
        return changed;
    }
    void invalidate_authority() noexcept { focused_ = false; owner_ = 0; authority_at_ = 0; balance_controls_ = {}; balance_frame_ = {}; cancel_commands(); }
    void publish_native_context(NativeInputFrame frame) noexcept {
        native_ = frame;
        if (!frame.valid) native_lip_ = {};
        // Existing physics production calls this for every validated step;
        // client rereads retain the original timestamp and step identity.
        native_rail_.observe(frame);
        sessions_[0].observe_manual_transition(InputNativeContext::from_frame(frame));
        // Central admission deliberately limits the candidate catch to verified
        // AIR. Rail contact is not a mapped darkslide or catch-setting proof.
        if (native_rail_method_ == RailAdapterMethod::dark_catch && (!frame.valid || !frame.airborne)) native_rail_.cancel();
    }
    // Read-only native fields. They never admit a named action or a Lip phase.
    void publish_native_lip(RailAdapterLipSnapshot observation, const NativeInputFrame& frame) noexcept {
        const auto& owner = observation.owner;
        // Both readers belong to one physics observation. A lease permits
        // consuming it later, never attaching old fields to a newer episode.
        const bool matches = observation.valid && frame.valid && frame.owner.valid() &&
            frame.owner_generation && frame.contact_generation && frame.action_generation && frame.physics_step &&
            owner.valid && owner.generation &&
            NativeInputOwner{owner.base,owner.entity,owner.core,owner.context,owner.rig} == frame.owner &&
            observation.physics_state == frame.state && observation.sampled_at == owner.sampled_at &&
            frame.sampled_at == observation.sampled_at;
        native_lip_ = matches ? observation : RailAdapterLipSnapshot{};
        native_lip_frame_ = matches ? frame : NativeInputFrame{};
    }
    RailAdapterLipSnapshot native_lip_observation(std::uint64_t now) const noexcept {
        const auto frame = native_context(now);
        return frame.valid && native_lip_.valid && now >= native_lip_.sampled_at && now-native_lip_.sampled_at <= 150 &&
            native_lip_frame_.owner == frame.owner && native_lip_frame_.owner_generation == frame.owner_generation &&
            native_lip_frame_.contact_generation == frame.contact_generation && native_lip_frame_.action_generation == frame.action_generation &&
            native_lip_frame_.physics_step && native_lip_frame_.physics_step <= frame.physics_step &&
            native_lip_frame_.sampled_at <= frame.sampled_at ? native_lip_ : RailAdapterLipSnapshot{};
    }
    // Invoked only after a newly consumed semantic request with its ORIGINAL
    // deadline. Channel compatibility is explicit; no method is activated by
    // held levels, a rail enum, or a missing named/Lip capability.
    RailAdapterResult request_native_rail(RailAdapterMethod method, std::uint64_t now,
            std::uint64_t deadline, bool channels_compatible) noexcept {
        const bool permitted = native_bound_ && native_context(now).valid && admitted(0,now) &&
            (method == RailAdapterMethod::dark_catch || !observed_lip_context(now));
        const auto result = native_rail_.request(method,now,deadline,enabled_,permitted,!channels_compatible);
        if (result == RailAdapterResult::started) native_rail_method_ = method;
        return result;
    }
    RailAdapterPad sample_native_rail(std::uint64_t now, bool held, bool channels_compatible) noexcept {
        const bool permitted = native_bound_ && native_context(now).valid && admitted(0,now) &&
            (native_rail_method_ == RailAdapterMethod::dark_catch || !observed_lip_context(now));
        return native_rail_.sample(now,held,enabled_,permitted,!channels_compatible);
    }
    RailAdapterPad sample_native_rail(const InputRailOutput& handoff, std::uint64_t now,
            bool channels_compatible) noexcept {
        const bool permitted = native_bound_ && native_context(now).valid && admitted(0,now) && channels_compatible && !observed_lip_context(now);
        const auto pad = native_rail_.sample(handoff,now,enabled_,permitted);
        if (pad.owns_right_stick) native_rail_method_ = RailAdapterMethod::align;
        return pad;
    }
    RailAdapterPhase native_rail_phase(std::uint64_t now) const noexcept {
        return native_context(now).valid ? native_rail_.phase() : RailAdapterPhase::unavailable;
    }
    NativeInputFrame native_context(std::uint64_t now) const noexcept {
        return enabled_ && focused_ && now >= authority_at_ && now-authority_at_ <= 150 &&
            native_.valid && native_.owner_generation == owner_ && now >= native_.sampled_at &&
            now-native_.sampled_at <= 150 ? native_ : NativeInputFrame{};
    }
    // Controls are admitted by the native mechanic owner, not inferred from
    // a rail enum or from the control being held. Bind them to its exact episode.
    void publish_balance_controls(InputBalanceControls controls, NativeInputFrame proof, bool history_corroborated = false) noexcept {
        const bool same_section = balance_controls_.admitted && controls.admitted && proof.valid &&
            balance_frame_.valid && proof.owner == balance_frame_.owner &&
            proof.owner_generation == balance_frame_.owner_generation &&
            proof.action_generation == balance_frame_.action_generation &&
            controls.kind == balance_controls_.kind && proof.physics_step >= balance_frame_.physics_step &&
            proof.sampled_at >= balance_frame_.sampled_at;
        const bool corroborated = history_corroborated || (same_section && balance_history_corroborated_);
        balance_controls_ = controls.admitted && proof.valid ? controls : InputBalanceControls{};
        if (balance_controls_.admitted)
            balance_controls_.generations = {proof.owner_generation,proof.contact_generation,proof.action_generation};
        balance_frame_ = proof;
        balance_history_corroborated_ = balance_controls_.admitted && corroborated;
    }
    NativeInputFrame balance_admission(std::uint64_t now, InputBalanceKind kind) const noexcept {
        const auto native = native_context(now);
        return native.valid && balance_controls_.admitted && balance_controls_.kind == kind &&
            balance_controls_current(now,native) &&
            balance_frame_.action_generation == native.action_generation ? balance_frame_ : NativeInputFrame{};
    }
    struct ManualCorrectionOutput { bool owns_y{}; std::int16_t y{}; };
    // The physical kernel publishes its actual protected force decision with
    // the original component observation. Input polls only consume that lease.
    void publish_manual_correction(float amount,const NativeInputFrame& proof,bool available) noexcept {
        manual_correction_known_=available && proof.valid && (amount==-1 || amount==0 || amount==1);
        manual_correction_=manual_correction_known_ ? amount : 0;
        manual_correction_frame_=manual_correction_known_ ? proof : NativeInputFrame{};
    }
    ManualCorrectionOutput manual_correction(std::uint64_t now,ManualBalanceButtons physical,bool compatible) const noexcept {
        const auto native=native_context(now);const auto& proof=manual_correction_frame_;
        const bool basic=balance_controls_.kind==InputBalanceKind::manual || balance_controls_.kind==InputBalanceKind::nose;
        if(!compatible || !manual_correction_known_ || !basic || !balance_controls_current(now,native) ||
            native.state!=100 || native.mode!=1 || (native.stance_and_push&0x40u) ||
            (native.contacts!=0xc000u && native.contacts!=0x4000u && native.contacts!=0x8000u) ||
            balance_frame_.action_generation!=native.action_generation || !proof.valid || !proof.owner.valid() ||
            !proof.owner_generation || !proof.contact_generation || !proof.action_generation ||
            proof.owner!=native.owner || !proof.physics_step || proof.physics_step>native.physics_step ||
            proof.sampled_at>native.sampled_at || now<proof.sampled_at || now-proof.sampled_at>150 ||
            proof.owner_generation!=native.owner_generation || proof.contact_generation!=native.contact_generation ||
            proof.action_generation!=native.action_generation)return {};
        // Release/opposite physical input withdraws an old force immediately;
        // only a later physical-kernel decision can select the new force.
        const auto current=manual_balance_correction(physical);
        return {true,manual_balance_native_y(current==manual_correction_ ? manual_correction_ : 0)};
    }
    const InputHistory* history(unsigned user) const noexcept {
        return user < sessions_.size() ? &sessions_[user].history() : nullptr;
    }
    void set_recognition_capture(unsigned user,bool enabled) noexcept {
        if(user<sessions_.size())sessions_[user].set_recognition_capture(enabled);
    }
    const UnifiedInput::Diagnostic* input_diagnostic(unsigned user) const noexcept {
        return user<sessions_.size()?&sessions_[user].diagnostic():nullptr;
    }
    std::uint64_t owner_epoch() const noexcept { return owner_; }
    bool admitted(unsigned user, std::uint64_t now) const noexcept {
        return enabled_ && (!native_bound_ || (user == 0 && focused_ && now >= authority_at_ && now-authority_at_ <= 150));
    }
    void set_input_policy(InputPolicy policy) noexcept {
        if (policy_ != policy) { cancel_commands(); policy_ = policy; }
    }
    InputPolicy input_policy() const noexcept { return policy_; }
    void set_playback_schedule(AuthoredPlaybackSchedule schedule) noexcept {
        for (auto& session : sessions_) session.set_schedule(schedule);
    }
    void enable(bool value) noexcept {
        if (enabled_ == value) return;
        enabled_ = value; cancel_commands();
        if (native_bound_) invalidate_authority();
    }
    std::int32_t authored_air_followup(std::uint64_t now,const NativeInputFrame& frame) const noexcept {
        return enabled_ && admitted(0,now) ? sessions_[0].authored_air_followup(now,InputNativeContext::from_frame(frame)) : 0;
    }
    bool pending_manual_revert(std::uint64_t now,const NativeInputFrame& frame) const noexcept {
        return enabled_ && admitted(0,now) && sessions_[0].pending_manual_revert(now,InputNativeContext::from_frame(frame));
    }
    void cancel_commands() noexcept { manual_correction_known_=false; for (auto& session : sessions_) session.reset(); ordinary_jump_playback_ = {}; native_rail_.cancel(); }
    // Ordinary jump and basic-flip recipes own RS, not the independent AIR
    // rotation channel. This is actor metadata, not native clip readiness,
    // completion, or a NoSpin observation.
    bool allows_air_rotation(unsigned user, bool authored_playback) const noexcept {
        return !authored_playback || (user < ordinary_jump_playback_.size() && ordinary_jump_playback_[user]);
    }
    void resume_manual_episode(const ManualInputDiagnostic& command,const NativeInputFrame& proof,std::uint64_t now) noexcept {
        const auto current=native_context(now);
        if(!command.owns || !current.valid || !proof.valid || current.owner!=proof.owner ||
            current.owner_generation!=proof.owner_generation || current.physics_step!=proof.physics_step)return;
        sessions_[0].resume_manual_episode(command,InputNativeContext::from_frame(proof));
    }
    void cancel_manuals() noexcept { manual_correction_known_=false; for (auto& session : sessions_) session.cancel_manual(); }
    bool initial_ground_acquisition(unsigned user, std::uint64_t now) const noexcept {
        const auto native=native_context(now);
        return user<sessions_.size() && admitted(user,now) && native.valid &&
            sessions_[user].initial_ground_acquisition(now,InputNativeContext::from_frame(native));
    }
    bool busy(unsigned user) const noexcept { return user < sessions_.size() && sessions_[user].busy(); }
    NativeGesture::Output poll(unsigned user, std::uint64_t now, std::uint16_t buttons,
            std::int16_t lx, std::int16_t ly, std::int16_t rx, std::int16_t ry,
            bool game_input, bool may_start, bool may_push = false, float speed = 0,
            bool triggers_held = false, bool may_toggle_board = false,
            bool authored = false, std::uint64_t tick = 0, bool nollie = false,
            std::span<const PreparedGesture> catalog = {}, bool mirror_x = false,
            bool manual_safe = false, SkatePose observed = SkatePose::unknown,
            bool = false, bool manual_prepare_only = false,
            bool offboard = false, bool action_busy = false, bool flip_ready = true, bool neutral_ready = false,
            std::uint8_t lt = 0, std::uint8_t rt = 0, ActionExperiment mode = ActionExperiment::off,
            bool action_fresh = false, bool inspected_stance = false, bool manual_balance_admitted = false,
            NativeFlipRequest normal_flip = {}) noexcept {
        if (!enabled_ || user >= sessions_.size() || (native_bound_ && user != 0)) return {buttons, rx, ry};
        if (native_bound_ && (!focused_ || now < authority_at_ || now-authority_at_ > 150 || !game_input)) {
            cancel_commands(); game_input = false;
        }
        // A retired/unissued native method must not leave a perpetual input
        // owner after its original readiness deadline. Active held methods
        // retain their existing lifetime, independently of that deadline.
        const auto expired_rail = native_bound_ && !native_rail_.active() ?
            sessions_[user].expire_inactive_rail(now) : InputRequest{};
        // Older callers can supply held-trigger admission without analog values.
        if (triggers_held && !lt && !rt) lt = 31;
        InputAdmission admission{game_input, may_start, may_push, may_toggle_board, authored,
            nollie, mirror_x, manual_safe, manual_prepare_only, offboard, action_busy,
            flip_ready, neutral_ready, action_fresh, inspected_stance, observed, mode, speed, tick, catalog};
        admission.manual_balance_admitted = manual_balance_admitted;
        admission.normal_flip=normal_flip;
        admission.input_policy = policy_;
        admission.export_rail_requests = native_bound_;
        const auto native = native_context(now);
        if (native_bound_) admission.native_context = InputNativeContext::from_frame(native);
        if (native_bound_ && !native.valid) admission.gameplay = false;
        if (native_bound_ && native.valid) {
            // Positive observed context is only a veto on generic rail input;
            // it does not supply a Lip actor, stage, balance or exit permission.
            admission.observed_lip = observed_lip_context(now);
            admission.offboard = native.state == game::build::v20260929::arcade_native_probe::offboard_state;
            const bool manual_phase = manual_balance_admitted &&
                (balance_admission(now,InputBalanceKind::manual).valid || balance_admission(now,InputBalanceKind::nose).valid);
            // An admitted basic balance selects its own jump family. Physical
            // LT is an ordinary-ground modifier, not a replacement balance identity.
            if (native.state == 100 && (buttons & classic_a)) {
                if (manual_phase) admission.nollie = balance_controls_.kind == InputBalanceKind::nose;
                else if (manual_balance_admitted && (observed == SkatePose::manual || observed == SkatePose::nose_manual))
                    admission.may_start = false; // Stale balance authority cannot substitute an ordinary hop.
            }
            // Caller poses can lag native landing/takeoff. Reconcile only the
            // verified states; contact alone never establishes balance identity.
            if (!admission.offboard && !native.rail_family) {
                if (native.state == 200 || native.state == 201) {
                    admission.pose = SkatePose::air;
                    admission.may_start = admission.may_push = admission.may_toggle_board = false;
                } else if (native.state == 100 || native.state == 103) {
                    admission.pose = native.state == 100 && manual_phase ?
                        (balance_controls_.kind == InputBalanceKind::nose ? SkatePose::nose_manual : SkatePose::manual) : SkatePose::riding;
                    admission.may_push = admission.may_push && native.state == 100 && native.mode == 1 && native.contacts == 0xc000u;
                    admission.manual_safe = admission.manual_safe && native.state == 100 && native.mode == 1;
                    admission.manual_prepare = false;
                } else if (native.state == 101 && native.mode <= 1) {
                    admission.pose = SkatePose::riding;
                    admission.may_start = admission.may_push = admission.manual_safe = admission.may_toggle_board = false;
                    admission.manual_prepare = admission.manual_prepare && native.mode == 1;
                } else if (native.state == 701) {
                    admission.pose = SkatePose::grind_exit_transition;
                    admission.may_start = admission.may_push = admission.manual_safe = admission.manual_prepare = admission.may_toggle_board = false;
                } else {
                    admission.pose = SkatePose::unknown; admission.gameplay = false;
                }
            }
            // The native400 family covers physical rail contact independently
            // of the current Manual/HUD observer. Named rail/Lip actor phases
            // remain unavailable; an observed Lip flag only vetoes generic input.
            if (native.rail_family) {
                admission.pose = SkatePose::grind;
                admission.may_start = admission.may_push = admission.manual_safe = admission.manual_prepare = false;
                admission.manual_balance_admitted = false;
            }
            admission.flip_ready = admission.flip_ready && native.flip_known && native.flip_id == -1;
            if (balance_controls_current(now,native)) {
                admission.balance_controls = balance_controls_;
                // Phase/identity admission needs all three generations; physical
                // correction exclusions retain their independent owner/contact binding.
                if (balance_frame_.action_generation == native.action_generation) {
                    admission.balance_generations = balance_controls_.generations;
                    admission.observed_lip = admission.observed_lip || balance_controls_.kind == InputBalanceKind::lip;
                }
            }
        }
        admission.manual_contact_only = native_bound_ && !(balance_history_corroborated_ &&
            balance_controls_current(now,native) && balance_frame_.action_generation == native.action_generation);
        auto output = sessions_[user].poll(now, {buttons, lx, ly, lt, rt}, rx, ry, admission);
        // Preserve one terminal receipt with its original identity/time. This
        // is cancellation, not a fabricated physical release or native issue.
        if (admission.gameplay && !admission.offboard && !admission.observed_lip &&
            output.rail.request.action == InputAction::none) output.rail.request = expired_rail;
        if (output.requested_id) {
            const bool basic_flip =
                output.requested_id == native_flip_id(ArcadeTrick::kickflip,false) ||
                output.requested_id == native_flip_id(ArcadeTrick::kickflip,true) ||
                output.requested_id == native_flip_id(ArcadeTrick::heelflip,false) ||
                output.requested_id == native_flip_id(ArcadeTrick::heelflip,true) ||
                output.requested_id == native_flip_id(ArcadeTrick::pop_shove_it,false) ||
                output.requested_id == native_flip_id(ArcadeTrick::pop_shove_it,true);
            ordinary_jump_playback_[user] = (output.requested_id == 1 || output.requested_id == 16 || basic_flip) &&
                !admission.offboard && admission.pose != SkatePose::grind && admission.pose != SkatePose::grind_exit_transition;
        }
        if (!output.authored_playback) ordinary_jump_playback_[user] = false;
        return output;
    }
private:
    bool observed_lip_context(std::uint64_t now) const noexcept {
        const auto observation = native_lip_observation(now);
        return observation.valid && (observation.fields & 2u) && observation.lip;
    }
    bool balance_controls_current(std::uint64_t now,const NativeInputFrame& native) const noexcept {
        // A generation is an invalidation counter, not sufficient owner proof.
        // Retain the component's original lease and physical observation order.
        return native.valid && balance_controls_.admitted && balance_frame_.valid &&
            balance_frame_.owner.valid() && balance_frame_.owner == native.owner &&
            balance_frame_.physics_step && balance_frame_.physics_step <= native.physics_step &&
            balance_frame_.sampled_at <= native.sampled_at && now >= balance_frame_.sampled_at &&
            now-balance_frame_.sampled_at <= 150 &&
            balance_frame_.owner_generation == native.owner_generation &&
            balance_frame_.contact_generation == native.contact_generation;
    }
    bool enabled_{}, native_bound_{}, focused_{};
    InputPolicy policy_;
    Physical physical_{};
    NativeInputFrame physical_frame_{};
    NativeInputFrame native_{}, balance_frame_{};
    RailAdapterLipSnapshot native_lip_{};
    NativeInputFrame native_lip_frame_{};
    RailAdapter native_rail_{};
    RailAdapterMethod native_rail_method_{};
    InputBalanceControls balance_controls_{};
    bool balance_history_corroborated_{};
    NativeInputFrame manual_correction_frame_{};
    float manual_correction_{};
    bool manual_correction_known_{};
    std::uint64_t owner_{}, authority_at_{};
    unsigned source_{};
    std::array<InputSession, 4> sessions_{};
    std::array<bool, 4> ordinary_jump_playback_{};
};
}
