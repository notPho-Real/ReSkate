#pragma once
#include "arcade_input_policy.h"
#include "arcade_input_types.h"
#include "Engine/Game/Build/20260929/launches.h"

namespace dingosdk::arcade_mode {
enum class RailAdapterMethod : std::uint8_t { align, nose_pressure, tail_pressure, dark_catch, named_grind, lip };
enum class RailAdapterResult : std::uint8_t { started, unsupported, unsafe, busy };
enum class RailAdapterPhase : std::uint8_t { unavailable, air, rail_contact };
struct RailAdapterPad {
    bool owns_right_stick{};
    std::int16_t right_x{}, right_y{};
    std::uint16_t button_mask{}, button_values{};
};
// A native input method, not a recognizer or a named-trick executor. Feed every
// validated physics frame to observe(), including non-rail states. Input polls
// can sample the cached frame but cannot renew its original timestamp.
class RailAdapter {
public:
    void cancel() noexcept { active_ = false; }
    void reset() noexcept { *this = {}; }
    void observe(InputNativeContext native, std::uint32_t state) noexcept {
        const bool air = state == 200 || state == 201;
        namespace rail = game::build::v20260929::launches;
        const bool contact = state >= rail::grind_states_first && state < rail::grind_states_end;
        const bool coherent = native.required && native.valid && native.airborne == air && native.rail_family == contact && native.follows(native_) &&
            (!native_.valid || native.physics_step != native_.physics_step || state == state_);
        if (!coherent || (native_.valid &&
            (native.generations.owner != native_.generations.owner ||
             (native.physics_step != native_.physics_step && native.physics_step - native_.physics_step != 1)))) cancel();
        if (!coherent || (!air && !contact)) cancel();
        native_ = coherent ? native : InputNativeContext{}; state_ = state;
        phase_ = !coherent ? RailAdapterPhase::unavailable : air ? RailAdapterPhase::air :
            contact ? RailAdapterPhase::rail_contact : RailAdapterPhase::unavailable;
    }
    template<class Frame> void observe(const Frame& frame) noexcept {
        observe(InputNativeContext::from_frame(frame), frame.state);
    }
    RailAdapterResult request(RailAdapterMethod method, std::uint64_t now,
            std::uint64_t deadline, bool enabled, bool admitted, bool other_owner) noexcept {
        // Unknown named variants never fall back to generic board pressure.
        if (method == RailAdapterMethod::named_grind || method == RailAdapterMethod::lip || method > RailAdapterMethod::lip)
            return RailAdapterResult::unsupported;
        if (!enabled || !admitted || !native_.fresh(now) || deadline < now ||
            phase_ == RailAdapterPhase::unavailable) return RailAdapterResult::unsafe;
        if (active_ || other_owner) return RailAdapterResult::busy;
        // Dark catch is the native RB input method around an actual air flip.
        // It does not establish a darkslide identity or rail approach geometry.
        if (method == RailAdapterMethod::dark_catch && phase_ != RailAdapterPhase::air)
            return RailAdapterResult::unsafe;
        method_ = method; deadline_ = deadline; active_ = true; sampled_ = false;
        return RailAdapterResult::started;
    }
    RailAdapterPad sample(std::uint64_t now, bool held, bool enabled, bool admitted, bool other_owner) noexcept {
        if (!active_) return {};
        if (!held || !enabled || !admitted || other_owner || !native_.fresh(now) || now > deadline_ ||
            phase_ == RailAdapterPhase::unavailable || (sampled_ && now < last_sample_)) { cancel(); return {}; }
        last_sample_ = now; sampled_ = true;
        if (method_ == RailAdapterMethod::dark_catch) return {false,0,0,0x0200,0x0200};
        const std::int16_t tilt = method_ == RailAdapterMethod::nose_pressure ? std::int16_t{20000} :
            method_ == RailAdapterMethod::tail_pressure ? std::int16_t{-20000} : std::int16_t{0};
        return {true,0,tilt,0,0};
    }
    // Consumes a semantic handoff; it never reads a journal or recognizes a
    // physical level as a new command. The original deadline gates first issue.
    RailAdapterPad sample(const InputRailOutput& handoff, std::uint64_t now,
            bool enabled, bool admitted) noexcept {
        const auto& request = handoff.request;
        const auto retire = [&]() noexcept { cancel(); semantic_retired_ = true; };
        if (request.action != InputAction::rail_request || !handoff.held || !enabled) { retire(); return {}; }
        if (request.session < semantic_.session || (request.session == semantic_.session &&
            request.input_sequence < semantic_.input_sequence)) { retire(); return {}; }
        if (request.session != semantic_.session || request.input_sequence != semantic_.input_sequence) {
            cancel(); semantic_ = request; semantic_started_ = semantic_retired_ = false;
        } else if (request != semantic_) { retire(); return {}; }
        if (semantic_retired_) return {};
        if (!request.session || !request.input_sequence || request.control != 0x8000 || request.presses != 1 ||
            request.variant || request.modifier || request.form != InputForm::ordinary ||
            (request.context != InputContext::ground && request.context != InputContext::air && request.context != InputContext::rail) ||
            !native_.fresh(now) || !request.generations.owner || request.generations.owner != native_.generations.owner ||
            now < request.recognized_at || now < request.physical_pressed_at) { retire(); return {}; }
        if (semantic_started_ && (!active_ || !admitted)) { retire(); return {}; }
        const bool occupied = handoff.blocked || handoff.occupied.owns_right_stick;
        if (occupied) { if (semantic_started_) retire(); return {}; }
        if (!semantic_started_) {
            if (!request.unbounded && (request.inclusive_deadline ? now > request.deadline : now >= request.deadline)) { retire(); return {}; }
            if (now < request.ready_at || !admitted || phase_ == RailAdapterPhase::unavailable) return {};
            RailAdapterMethod method{};
            switch (request.direction) {
            case 0: method = RailAdapterMethod::align; break;
            case 8: method = RailAdapterMethod::nose_pressure; break;
            case 2: method = RailAdapterMethod::tail_pressure; break;
            default: retire(); return {};
            }
            // Accepted held actuation has no renewed recognition lease. Native
            // continuity, physical release and explicit admission end ownership.
            if (this->request(method,now,UINT64_MAX,true,true,false) != RailAdapterResult::started) return {};
            semantic_started_ = true;
        }
        return sample(now,true,true,true,false);
    }
    bool active() const noexcept { return active_; }
    RailAdapterPhase phase() const noexcept { return phase_; }
private:
    InputNativeContext native_{};
    InputRequest semantic_{};
    bool semantic_started_{}, semantic_retired_{};
    RailAdapterMethod method_{};
    RailAdapterPhase phase_{};
    std::uint32_t state_{};
    std::uint64_t deadline_{}, last_sample_{};
    bool active_{}, sampled_{};
};
}
