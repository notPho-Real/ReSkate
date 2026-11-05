#pragma once
#include <cmath>
#include <cstdint>
namespace dingosdk::arcade_mode {
template<class Frame>
bool held_flip_continues(const Frame& before,const Frame& current,std::uint64_t now,std::int32_t id) noexcept {
    const auto air_mode=[](const auto& flip) noexcept {
        return (flip.state==200 && flip.mode<=2) || (flip.state==201 && flip.mode>=1 && flip.mode<=2);
    };
    if (!before.required || !current.required || !before.fresh(now) || !current.fresh(now) ||
        !before.airborne || !current.airborne || before.air_episode()!=current.air_episode() ||
        before.generations.owner!=current.generations.owner || !before.flip.valid || !current.flip.valid ||
        !before.flip.object || before.flip.object!=current.flip.object || before.flip.stance!=current.flip.stance ||
        (current.flip.stance&0x40u) ||
        (before.flip.state!=current.flip.state && !(before.flip.state==200 && current.flip.state==201)) ||
        !air_mode(before.flip) || !air_mode(current.flip) ||
        (before.flip.id==id ? current.flip.id!=id :
            (before.flip.id!=1 && before.flip.id!=-1) || (current.flip.id!=1 && current.flip.id!=-1 && current.flip.id!=id))) return false;
    const unsigned action_change=before.flip.state!=current.flip.state || before.flip.id!=current.flip.id;
    const unsigned mode_change=before.flip.mode!=current.flip.mode;
    return current.generations.action>=before.generations.action &&
        current.generations.action-before.generations.action==action_change &&
        current.flip.mode_generation>=before.flip.mode_generation &&
        current.flip.mode_generation-before.flip.mode_generation==mode_change;
}
// Native reads only. Count is additional rotations: stock Double Kickflip=1.
// Input playback cannot acknowledge an actor or manufacture this observation.
struct HeldFlipObservation {
    bool available{}, valid{}, prepare{}, holding{}, actor_advanced{};
    std::int32_t id{};
    float extra_rotations{};
    std::uint64_t sampled_at{};
    bool fresh(std::uint64_t now) const noexcept {
        return available && valid && now>=sampled_at && now-sampled_at<=150 &&
            std::isfinite(extra_rotations) && extra_rotations>=0 && extra_rotations<=32;
    }
};
class HeldFlipInput {
public:
    bool start(std::uint64_t now,std::int32_t id,const HeldFlipObservation& native) noexcept {
        if (active_ || (id!=2 && id!=3) || !native.fresh(now) || native.id!=id ||
            native.holding || native.extra_rotations!=0) return false;
        *this={};active_=true;id_=id;began_=now;prepared_=native.prepare;return true;
    }
    bool hold(std::uint64_t now,const HeldFlipObservation& native,bool same_flight) noexcept {
        if (!active_) return false;
        if (!same_flight || !native.fresh(now) || native.id!=id_ || now<began_ || now-began_>1200) {
            active_=false;return false;
        }
        entered_=entered_ || native.holding;
        prepared_=prepared_ || native.prepare;
        actor_=actor_ || native.actor_advanced;
        // The original native counter counts EXTRA rotations. Withdrawal at
        // one requests the game's normal double catch, never writes a count.
        if (native.extra_rotations>=1) {
            acknowledged_=native.extra_rotations==1 && entered_ && actor_;
            active_=false;return false;
        }
        // The original gesture must be recognized before EAT tests PrepareHeldFlip.
        // Starting the flick only when that short window opens misses admission.
        // Preparation gets the existing 200ms input-readiness bound, never an
        // animation extension. A window that closes without admission still ends.
        if ((!prepared_ && now-began_>200) ||
            (prepared_ && !native.prepare && !native.holding) ||
            (entered_ && !native.holding)) active_=false;
        return active_;
    }
    bool active() const noexcept {return active_;}
    bool acknowledged() const noexcept {return acknowledged_;}
    void cancel() noexcept {*this={};}
private:
    std::uint64_t began_{};
    std::int32_t id_{};
    bool active_{},prepared_{},entered_{},actor_{},acknowledged_{};
};
}
