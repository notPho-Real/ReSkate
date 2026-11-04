#pragma once
#include "arcade_authored_playback.h"
#include "arcade_native_grab_lease.h"

namespace dingosdk::arcade_mode::native_grab {
struct FingerflipRow {
    int identity, family; unsigned gestures; int destination;
};
// CDB_OnBoard_Grab_FingerFlip_Into. The bitmap uses the original
// FliptrickTypeForFingerflip enum, not ordinary flip or grab IDs.
inline constexpr FingerflipRow fingerflips[]{
    {1,6,2,6},{2,8,2,8},{3,8,56,2},{4,4,62,6},{5,7,56,1},{6,7,2,7},
    {7,3,2,3},{8,5,62,3},{9,2,16,8},{10,2,40,2},{11,2,2,2},
    {12,1,16,7},{13,1,40,1},{14,1,2,1},
    {19,9,2,9},{20,9,4,9},{21,9,16,9},{22,9,40,9},
    {23,2,4,2},{24,1,4,1},{25,6,4,6},{26,3,4,3},{27,8,4,8},
    {28,7,4,7},{29,3,56,5},{30,6,56,4}
};
constexpr unsigned finger_gesture(int native_flip) noexcept {
    return native_flip==2?2u:native_flip==3?4u:native_flip==4?8u:native_flip==7?16u:0u;
}
constexpr const FingerflipRow* fingerflip_row(int family,int native_flip) noexcept {
    const auto gesture=finger_gesture(native_flip);
    for(const auto& row:fingerflips) if(row.family==family && (row.gestures&gesture))return &row;
    return nullptr;
}
constexpr ArcadeTrick grab_family_trick(int family) noexcept {
    switch(family) {
    case 1:return ArcadeTrick::indy;case 2:return ArcadeTrick::basic_bs_grab;
    case 3:return ArcadeTrick::tailgrab;case 4:return ArcadeTrick::seatbelt;
    case 5:return ArcadeTrick::crail;case 6:return ArcadeTrick::nosegrab;
    case 7:return ArcadeTrick::stale;case 8:return ArcadeTrick::weddle;
    case 9:return ArcadeTrick::double_grab;default:return ArcadeTrick::none;
    }
}
struct FingerflipObservation {
    bool valid{},in_fingerflip{},grabbing{},can_grab{};
    int identity{},family{};
};
// Own only the original gesture and hand inputs. EAT owns its late-flip
// buffer, the CDB owns the identity/destination, and StateFlow owns the actor.
class FingerflipInput {
public:
    static constexpr std::uint64_t entry_ms=800, lifetime_ms=2500, heartbeat_ms=150;
    bool start(const Pilot& parent,bool parent_acknowledged,const RetargetObservation& observed,
               int flip,std::span<const AuthoredStickStep> steps,std::uint64_t now,std::uint64_t tick,bool mirror) noexcept {
        if(active() || parent.one_footer || !parent_acknowledged || !observed.eligible(&parent))return false;
        const auto* row=fingerflip_row(parent.family,flip);
        if(!row || !playback_.start(steps,tick,false,mirror,true))return false;
        std::copy(steps.begin(),steps.end(),steps_.begin());size_=steps.size();mirror_=mirror;
        playback_.cancel();row_=row;parent_=&parent;began_=last_=now;last_tick_=tick;
        control_={parent.hands,0,0,0,true};return true;
    }
    bool acknowledge(int identity,bool initialized,bool exact,float delta,float before,float after) noexcept {
        if(!row_ || identity!=row_->identity || !initialized || !exact ||
            !std::isfinite(delta) || delta<=0 || !std::isfinite(before) || !std::isfinite(after) || after<=before)return false;
        acknowledged_=true;return true;
    }
    Control poll(std::uint64_t now,std::uint64_t tick,bool settings,const FingerflipObservation& observed) noexcept {
        if(!row_)return {};
        if(!observed.valid || now<last_ || now-last_>heartbeat_ms || tick<last_tick_ ||
            now-began_>lifetime_ms || (!acknowledged_ && now-began_>entry_ms)) {cancel();return {};}
        last_=now;last_tick_=tick;
        if(observed.in_fingerflip)entered_=true;
        if(started_ && !entered_ && !settings) {cancel();return {};}
        if(observed.in_fingerflip && observed.identity && observed.identity!=row_->identity) {cancel();return {};}
        // Stop the gesture as soon as the native actor accepts it, avoiding
        // another buffer entry. Neutral is explicitly owned during the catch.
        if(entered_)playback_.cancel();
        if(acknowledged_ && observed.family==row_->destination)destination_seen_=true;
        if(entered_ && acknowledged_ && destination_seen_ && !observed.in_fingerflip &&
            observed.grabbing && observed.can_grab)completed_=true;
        const auto* hold=destination_seen_?pilot(grab_family_trick(row_->destination)):parent_;
        Control out{};out.hands=hold->hands;out.transition=true;
        if(settings && !entered_) {
            if(!started_) {playback_.start({steps_.data(),size_},tick,false,mirror_,true);started_=true;}
            const auto stick=playback_.sample(tick,now);
            out.x=stick.x;out.y=stick.y;
        }
        control_=out;return out;
    }
    bool active()const noexcept{return row_!=nullptr;}
    bool acknowledged()const noexcept{return acknowledged_;}
    bool completed()const noexcept{return completed_;}
    const FingerflipRow* row()const noexcept{return row_;}
    Control control()const noexcept{return control_;}
    void cancel()noexcept{*this={};}
private:
    const FingerflipRow* row_{};const Pilot* parent_{};AuthoredPlayback playback_{};
    std::array<AuthoredStickStep,128> steps_{};std::size_t size_{};
    Control control_{};
    std::uint64_t began_{},last_{},last_tick_{};
    bool entered_{},acknowledged_{},destination_seen_{},completed_{},started_{},mirror_{};
};
}
