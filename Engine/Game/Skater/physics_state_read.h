#pragma once
#include <cstdint>

namespace dingosdk::game::skater {
struct PhysicsStateRead {
    std::uint32_t first{}, state{};
    bool readable{}, rechecked{}, owner_verified{true};
    bool known() const noexcept { return readable && owner_verified && state != 0; }
};

// The caller has already qualified its owner. Independent client captures have
// reported zero immediately before a verified nonzero state. Revalidate that exact
// owner around one new read; never reuse a previous frame or wait for a state.
// This is still not a transactional snapshot. A persistent zero, failed read
// or changed owner remains unknown, and every nonzero result keeps its value.
template<class ReadState, class SameOwner>
PhysicsStateRead read_current_physics_state(ReadState&& read_state, SameOwner&& same_owner) noexcept {
    PhysicsStateRead out{};
    out.readable=read_state(out.state);
    out.first=out.state;
    if (!out.readable || out.state!=0) return out;
    out.rechecked=true;
    if (!same_owner()) { out.owner_verified=false; return out; }
    out.readable=read_state(out.state);
    out.owner_verified=same_owner();
    return out;
}
}
