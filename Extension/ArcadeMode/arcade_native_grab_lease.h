#pragma once
#include "arcade_trick_table.h"
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode::native_grab {
// These are Enum.DetailedGrabType values emitted by the original loop actors,
// not Enum.GrabType values and not requested animation IDs.
struct IdentityObservation {
    std::int32_t detail{},one_footer{};
    bool valid{};
};
template<class ReadIdentity>
IdentityObservation observe_identity(const Pilot& profile,ReadIdentity&& read_identity) noexcept {
    IdentityObservation out{};
    const bool footer=profile.one_footer!=0;
    // OneFooterType is independent of DetailedGrabType. A first one-footer
    // can initialize before the ordinary detailed-grab field is allocated.
    out.valid=read_identity(footer,footer?out.one_footer:out.detail);
    return out;
}
// Selector RS coordinates preserve the original qualified-stance producer.
// They select the family before the original blendspace applies tweak X/Y;
// no hand, family enum, animation ID or score is written by this control map.
inline constexpr auto pilots=[] {
    constexpr auto count=[] {std::size_t n=0;for(const auto& t:trick_registry)n+=t.grab_variant_count;return n;}();
    std::array<Pilot,count> result{};
    for(const auto& t:trick_registry)for(unsigned i=0;i<t.grab_variant_count;++i) {
        const auto& variant=t.grab_variants[i];result[variant.actor_index]=variant;
    }
    return result;
}();
constexpr ArcadeTrick extended_grab(std::int16_t direction,ArcadeTrick selected=ArcadeTrick::double_grab) noexcept {
    return extended_grab_binding(trick_registry,direction,selected);
}
constexpr const Pilot* pilot(ArcadeTrick trick) noexcept {
    for (const auto& p:pilots) if (p.trick==trick) return &p;
    return nullptr;
}

// Original native grab signals, read together from the qualified local state
// block. BS here is EnumGS.GrabType=2, not DetailedGrabType=2 (Method).
struct RetargetObservation {
    bool valid{},grabbing{},can_grab{},left{},right{};
    std::int32_t family{};
    bool selector_ready{}; // Original input producer has observed this lease's family selector.
    bool eligible(const Pilot* selected=nullptr) const noexcept {
        const auto hands=selected?selected->hands:2u;
        return valid && grabbing && can_grab && left==((hands&1u)!=0) &&
            right==((hands&2u)!=0) && family==(selected?selected->family:2);
    }
};

// The gamepad producer applies two user preferences to X/A. A foot lease
// temporarily requests their unambiguous physical-foot configuration, then
// waits until the existing settings getter has actually returned it. Both-feet
// uses the native slide/B input, never simultaneous X+A (the hippy-jump input).
constexpr std::uint16_t foot_button(unsigned foot) noexcept {
    return foot==1?0x4000:foot==2?0x1000:foot==3?0x2000:0;
}

// A physical request remains pending through a brief release until independent
// native actor advancement AND the native dominant-pose tag agree. The deadline
// is a failure bound, never a required input pulse or animation setup duration.
class Lease {
public:
    static constexpr std::uint64_t initialization_limit_ms=1500, heartbeat_ms=150;
    bool request(ArcadeTrick trick,std::uint64_t now,bool held) noexcept {
        const auto* p=pilot(trick);
        if (!p) return false;
        selected_=p; requested_=refreshed_=now; released_=!held; acknowledged_=false; feet_started_=false; transition_=false;
        return true;
    }
    void refresh(std::uint64_t now,bool held,bool same_air_owner) noexcept {
        if (!selected_) return;
        if (!same_air_owner || now<refreshed_ || now-refreshed_>heartbeat_ms) { cancel();return; }
        refreshed_=now;
        // A subsequent physical hold cannot resurrect this physical occurrence.
        released_=released_ || !held;
        if (!acknowledged_ && !transition_ && now-requested_>initialization_limit_ms) cancel();
    }
    bool acknowledge(std::int32_t detail,float delta,float before,float after,
                     bool initialized,bool exact_actor,bool native_grabbing,std::int32_t one_footer=0) noexcept {
        if (!selected_ || (selected_->one_footer ? !feet_started_ || one_footer!=selected_->one_footer : detail!=selected_->detail) ||
            !initialized || !exact_actor || !native_grabbing ||
            !std::isfinite(delta) || delta<=0 || !std::isfinite(before) || !std::isfinite(after) || after<=before) return false;
        acknowledged_=true;return true;
    }
    bool intent(std::uint64_t now) const noexcept {
        return selected_ && now>=refreshed_ && now-refreshed_<=heartbeat_ms &&
            (acknowledged_ || transition_ || (now>=requested_ && now-requested_<=initialization_limit_ms)) &&
            !(released_ && (acknowledged_ || transition_));
    }
    bool physical_hold(std::uint64_t now) const noexcept {return !released_ && intent(now);}
    // A separately verified native transition has its own admission deadline.
    // Keep only its physical hold, without falsely acknowledging the old pose.
    void begin_transition() noexcept {transition_=true;}
    bool acknowledged() const noexcept {return acknowledged_;}
    Control controls(const RetargetObservation& native,bool settings_applied) noexcept {
        if (!selected_) return {};
        auto out=selected_->control();
        // The native hand producer can run before its RS selector producer.
        // Wait for the actual selector, not an elapsed controller-frame delay.
        // An established matching family already proves selection.
        if (out.selector() && !native.selector_ready &&
            !(native.valid && native.grabbing && native.family==selected_->family)) {
            out.hands=0;return out;
        }
        if (selected_->one_footer && settings_applied) {
            // CanOneFoot is owned by the original foot producer and may depend
            // on this input. Require the established grab, not that result.
            if (native.eligible(selected_)) feet_started_=true;
            if (feet_started_) out.feet=foot_button(selected_->foot);
        }
        return out;
    }
    const Pilot* selected() const noexcept {return selected_;}
    void cancel() noexcept {*this={};}
private:
    const Pilot* selected_{};
    std::uint64_t requested_{},refreshed_{};
    bool released_{},acknowledged_{},feet_started_{},transition_{};
};
}
