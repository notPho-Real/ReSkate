#pragma once
#include "arcade_native_input_state.h"
#include "arcade_revert_input.h"
#include "arcade_spin_control.h"
#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

namespace dingosdk::arcade_mode {
enum class GroundActionKind : std::uint8_t { unknown, switch_animation, powerslide_180, landing_revert, manual_link };
// Semantic catalog identity only; it supplies no native asset or actor id.
constexpr std::string_view ground_action_name(GroundActionKind kind) noexcept {
    return kind==GroundActionKind::landing_revert ? "Revert" : std::string_view{};
}
inline std::optional<bool> backwards_facing_change(float speed_squared,float facing_dot,std::optional<float> flip_speed,
        SpinFact bailing,SpinFact skitching,SpinFact braking) noexcept {
    if(bailing==SpinFact::set || skitching==SpinFact::set || braking==SpinFact::set)return false;
    if(bailing==SpinFact::unknown || skitching==SpinFact::unknown || braking==SpinFact::unknown || !flip_speed ||
        !std::isfinite(*flip_speed) || *flip_speed<0 || !std::isfinite(speed_squared) || speed_squared<0 || !std::isfinite(facing_dot))return {};
    return facing_dot<0 && speed_squared>(*flip_speed)*(*flip_speed);
}
enum class SetupStance : std::uint8_t { unknown, normal, pressure, nollie };
constexpr SetupStance next_setup_stance(SetupStance value) noexcept {
    return value==SetupStance::normal ? SetupStance::pressure : value==SetupStance::pressure ? SetupStance::nollie :
        value==SetupStance::nollie ? SetupStance::normal : SetupStance::unknown;
}
constexpr SetupStance reset_setup_stance(SetupStance value,GroundActionKind action) noexcept {
    if(action==GroundActionKind::landing_revert)return SetupStance::normal;
    if((action==GroundActionKind::switch_animation || action==GroundActionKind::powerslide_180) && value==SetupStance::nollie)return SetupStance::normal;
    return value;
}
inline bool ground_component_current(const NativeInputFrame& proof,const NativeInputFrame& current,std::uint64_t now) noexcept {
    return proof.valid&&current.valid&&proof.owner.valid()&&proof.owner==current.owner&&
        proof.owner_generation&&proof.contact_generation&&proof.action_generation&&proof.physics_step&&
        proof.owner_generation==current.owner_generation&&proof.contact_generation==current.contact_generation&&proof.action_generation==current.action_generation&&
        proof.physics_step<=current.physics_step&&proof.sampled_at<=current.sampled_at&&now>=proof.sampled_at&&now-proof.sampled_at<=150&&
        now>=current.sampled_at&&now-current.sampled_at<=150;
}
// Remembers only positively observed native setup state. A request cannot write
// native stance, cancel a Manual, or commit an unacknowledged transition.
class SetupStanceSelection {
public:
    bool observe(SetupStance state,const NativeInputFrame& proof,const NativeInputFrame& current,std::uint64_t now) noexcept {
        if(state==SetupStance::unknown || !ground_component_current(proof,current,now))return false;
        state_=state;proof_=proof;return true;
    }
    SetupStance remembered() const noexcept {return state_;}
    std::optional<SetupStance> request_cycle(const NativeInputFrame& current,std::uint64_t now,
                                            bool native_transition_supported,bool manual_or_other_owner) const noexcept {
        if(!native_transition_supported || manual_or_other_owner || !ground_component_current(proof_,current,now) ||
            (current.state!=100&&current.state!=103))return {};
        const auto next=next_setup_stance(state_);return next==SetupStance::unknown ? std::nullopt : std::optional{next};
    }
    void owner_lost() noexcept {state_=SetupStance::unknown;proof_={};}
private:
    SetupStance state_{};NativeInputFrame proof_{};
};
// Side flags are native observations, never inferred from a numeric actor id.
inline std::optional<RevertSide> physical_revert_side(SpinFact front,SpinFact back,
                                                    std::optional<RevertSide> last_physical_spin) noexcept {
    if(front==SpinFact::set)return RevertSide::frontside;
    if(front==SpinFact::unknown)return {};
    if(back==SpinFact::set)return RevertSide::backside;
    if(back==SpinFact::unknown)return {};
    return last_physical_spin;
}
class RevertFriction {
public:
    void physical_end() noexcept {index_=0;stamp_=0;}
    unsigned index() const noexcept {return index_;}
    void tick(std::uint64_t now) noexcept {
        if(index_ && now>=stamp_ && now-stamp_>20000){--index_;if(index_)stamp_=now;}
    }
    float select(std::uint64_t now) noexcept {
        tick(now);index_=std::min(index_,5u);
        constexpr std::array coefficients{0.f,0.f,5.f,10.f,15.f,25.f};
        const float coefficient=coefficients[index_];if(index_==0)stamp_=now;++index_;return coefficient;
    }
private:
    unsigned index_{};std::uint64_t stamp_{};
};
enum class NativeGroundPhase : std::uint8_t { unknown, initialized, animation_finished, exited };
struct NativeGroundActionObservation {
    NativeInputFrame frame{};
    GroundActionKind kind{};
    std::uintptr_t actor{}; // Requires an actual supported native binding.
    std::uint64_t occurrence{},physical_elapsed_ms{};
    NativeGroundPhase phase{};
    bool actor_known{},elapsed_known{};
};
struct PhysicalRevertState {
    bool available{},active{},can_kick{},ollied_exception{},friction_override{},restore_tables{};
    bool request_nollie_off{},request_pressure_off{};
    float rolling_friction{};
    std::optional<RevertSide> side{};
};
// This policy is inert without a verified actor occurrence, original phase
// observation and physical elapsed clock. The existing235ms pad candidate is
// neither an initialization observation nor an animation-completion signal.
class PhysicalRevert {
public:
    bool begin(const NativeGroundActionObservation& observed,const NativeInputFrame& current,std::uint64_t now,
               SpinFact front,SpinFact back,std::optional<RevertSide> last_spin,bool other_owner) noexcept {
        const auto side=physical_revert_side(front,back,last_spin);
        if(active_ || other_owner || !side || !qualified(observed,current,now) || now<observed.physical_elapsed_ms ||
            observed.kind!=GroundActionKind::landing_revert || observed.phase!=NativeGroundPhase::initialized ||
            (observed.actor==spent_actor_&&observed.occurrence==spent_occurrence_&&observed.frame.owner_generation==spent_owner_))return false;
        observation_=observed;side_=side;coefficient_=friction_.select(now-observed.physical_elapsed_ms);active_=event_known_=true;
        spent_actor_=observed.actor;spent_occurrence_=observed.occurrence;spent_owner_=observed.frame.owner_generation;return true;
    }
    PhysicalRevertState sample(const NativeGroundActionObservation& observed,const NativeInputFrame& current,std::uint64_t now) noexcept {
        friction_.tick(now);
        if(!active_ || !qualified(observed,current,now) || observed.kind!=GroundActionKind::landing_revert ||
            observed.actor!=observation_.actor || observed.occurrence!=observation_.occurrence ||
            observed.frame.owner!=observation_.frame.owner || observed.frame.owner_generation!=observation_.frame.owner_generation ||
            observed.frame.physics_step<observation_.frame.physics_step || observed.physical_elapsed_ms<observation_.physical_elapsed_ms){active_=false;return {};}
        // A repeated physics observation cannot acquire a later stage clock.
        if(observed.frame.physics_step==observation_.frame.physics_step &&
            (observed.physical_elapsed_ms!=observation_.physical_elapsed_ms || observed.phase!=observation_.phase)){active_=false;return {};}
        observation_=observed;
        const bool finished=observed.phase==NativeGroundPhase::animation_finished && observed.physical_elapsed_ms>=100;
        const bool exited=observed.phase==NativeGroundPhase::exited;
        if(finished||exited){active_=false;return {true,false,true,false,observed.physical_elapsed_ms<667,finished,false,false,coefficient_,side_};}
        return {true,true,false,observed.physical_elapsed_ms>=100,observed.physical_elapsed_ms<667,false,true,true,coefficient_,side_};
    }
    // Special friction is independent of animation completion. The caller
    // supplies an original, independently known physical clock for that event.
    std::optional<float> rolling_friction(const NativeInputFrame& current,std::uint64_t now,
                                         bool physical_clock_known,std::uint64_t elapsed_ms) const noexcept {
        if(!event_known_ || !physical_clock_known || elapsed_ms<observation_.physical_elapsed_ms || elapsed_ms>=667 ||
            !current.valid || !current.owner.valid() || current.owner!=observation_.frame.owner ||
            current.owner_generation!=observation_.frame.owner_generation || !current.physics_step ||
            current.physics_step<observation_.frame.physics_step || now<current.sampled_at || now-current.sampled_at>150)return {};
        return coefficient_;
    }
    void physical_end() noexcept {active_=event_known_=false;friction_.physical_end();}
    unsigned friction_index() const noexcept {return friction_.index();}
private:
    static bool qualified(const NativeGroundActionObservation& observed,const NativeInputFrame& current,std::uint64_t now) noexcept {
        return observed.actor_known&&observed.actor&&observed.occurrence&&observed.elapsed_known&&observed.phase!=NativeGroundPhase::unknown&&
            ground_component_current(observed.frame,current,now);
    }
    NativeGroundActionObservation observation_{};RevertFriction friction_{};std::optional<RevertSide> side_{};
    float coefficient_{};bool active_{},event_known_{};std::uintptr_t spent_actor_{};std::uint64_t spent_occurrence_{},spent_owner_{};
};
// Only the explicitly selected powerslide script has a300ms timed half-turn.
// Ordinary switch, landing Revert and ManualLink need their own actor/stage.
inline std::optional<float> powerslide_half_turn(GroundActionKind action,std::uint64_t elapsed_ms) noexcept {
    if(action!=GroundActionKind::powerslide_180)return {};
    return static_cast<float>(TapSpin::pi*static_cast<double>(std::min<std::uint64_t>(elapsed_ms,300))/300);
}
}
