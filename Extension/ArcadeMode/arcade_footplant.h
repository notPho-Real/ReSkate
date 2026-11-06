#pragma once
#include <array>
#include <cstdint>

namespace dingosdk::arcade_mode {
enum class Footplant : std::uint8_t { none, no_comply, boneless };
struct FootplantObservation {
    bool valid{}, can_no_comply{}, no_comply{}, boneless{}, left_hand{}, right_hand{};
    std::int32_t grab{};
    unsigned state{};
    std::uint64_t sampled_at{},step{};
    bool fresh(std::uint64_t now) const noexcept {
        return valid && step && now>=sampled_at && now-sampled_at<=150;
    }
};
struct FootplantControl {
    bool owned{};
    std::uint16_t buttons{};
    std::uint8_t left_trigger{};
    std::int16_t right_y{};
};
// A bounded native input owner. The game establishes its push/grab prerequisite
// before this owner supplies the entry input. No state, identity or launch write.
class FootplantInput {
public:
    bool start(Footplant kind,std::uint64_t now,const FootplantObservation& native) noexcept {
        if (active() || (kind!=Footplant::no_comply && kind!=Footplant::boneless) ||
            !native.fresh(now) || native.state!=100 || native.no_comply || native.boneless) return false;
        *this={};kind_=kind;started_=now;last_step_=native.step;return true;
    }
    FootplantControl poll(std::uint64_t now,const FootplantObservation& native,bool settings) noexcept {
        if (!active()) return {};
        // Up may already have started a regular push. The original animation
        // finishes that push before changing feet (measured at about1.43s),
        // so No Comply must allow its native mongo prerequisite to arrive.
        const auto preparation_limit=kind_==Footplant::no_comply?2000u:1000u;
        if (!native.fresh(now) || native.step<last_step_ || now<started_ || now-started_>preparation_limit ||
            (native.state!=100 && native.state!=103 && native.state!=602)) {cancel();return {};}
        last_step_=native.step;
        if ((kind_==Footplant::no_comply && native.no_comply) ||
            (kind_==Footplant::boneless && native.boneless && native.state==602)) {
            // The original action owns the launch and cleanup after entry.
            entered_=issued_;kind_=Footplant::none;return {};
        }
        if ((kind_==Footplant::no_comply && native.state==602) ||
            (kind_==Footplant::boneless && native.state==103)) {cancel();return {};}
        FootplantControl out{true};
        if (!settings) { if(prepared_)cancel();return active()?out:FootplantControl{}; }
        const bool prerequisite=kind_==Footplant::no_comply ? native.can_no_comply :
            native.left_hand && !native.right_hand && native.grab==1;
        // A fresh native observation must follow an actual preparation output;
        // pre-existing state cannot turn the initial sample into an entry edge.
        if (!prepared_) {prepared_=true;prepared_at_=now;prepared_step_=native.step;}
        else if (!issued_ && prerequisite && native.sampled_at>prepared_at_ && native.step>prepared_step_) {
            issued_=true;issued_at_=now;
        }
        if (issued_ && (now<issued_at_ || now-issued_at_>300)) {cancel();return {};}
        if (kind_==Footplant::no_comply) {
            out.buttons=0x1000; // Front-foot push with the scoped native settings.
            if (issued_) out.right_y=-28000;
        } else {
            out.left_trigger=255;
            if (issued_) out.buttons=0x1000; // Fresh plant input after original FS selection.
        }
        return out;
    }
    Footplant kind() const noexcept {return kind_;}
    std::array<bool,3> native_options() const noexcept {
        // The original swapped A/X preset makes A the front-foot push when
        // front/back controls are enabled. Without it A starts a regular push,
        // whose animation never opens the native CanNoComply tag window.
        const bool no_comply=kind_==Footplant::no_comply;
        return {no_comply,no_comply,no_comply};
    }
    bool active() const noexcept {return kind_!=Footplant::none;}
    bool entered() const noexcept {return entered_;}
    bool preparation_started() const noexcept {return prepared_;}
    bool entry_issued() const noexcept {return issued_;}
    void cancel() noexcept {*this={};}
private:
    Footplant kind_{};
    std::uint64_t started_{},prepared_at_{},issued_at_{},prepared_step_{},last_step_{};
    bool prepared_{},issued_{},entered_{};
};
}
