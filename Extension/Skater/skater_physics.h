#pragma once
#include "Engine/Game/Skater/skater_body.h"
#include "Engine/Game/Build/20260929/skater_body.h"
#include "Engine/Game/Build/20260929/skater_state.h"
#include "Extension/Skater/no_bail.h"
#include <cmath>
#include <cstdint>
#include <optional>

// The local skater as its physics simulates it, every physics step No Bail hands on (no_bail.h): what
// each body touched (Engine/Game/Build/20260929/skater_body.h) and how the skater moves, on the board
// or off it and whether in a ragdoll (Engine/Game/Build/20260929/skater_state.h). Plugins hear it as
// the physics_step event (skater_events.h). Read-only, but for the physics step length the game speed
// (game_speed.h) gives the skater.
namespace dingosdk::skater_physics {
// One physics step of the local skater.
struct Step {
    float seconds{}; // how long the step simulated, in the game's time
    std::optional<std::uint32_t> state; // native selector state; no memory identity
    bool wipeout{};  // the step asks for a wipeout
    bool airborne{}; // the skater is in the air, on the board or off it
    // The body is a ragdoll; empty when the skater state could not be read.
    std::optional<bool> ragdoll;
    // How the skater moves (with the board while on it, else as its pelvis), metres per second;
    // empty when unknown.
    std::optional<game::Vec3> velocity;
    // How fast the pelvis turns, any way round, radians per second; empty when unknown.
    std::optional<float> spin;
    struct Pose { std::array<float, 16> transform{}; game::Vec3 velocity{}; };
    // Rig bones1..23. The board_root slot stays empty: it is not rig body0.
    std::array<std::optional<Pose>, skater_body::count> poses;
    // Board physics body8, the native orientation reference. This is a
    // separate nine-body pool: rig bone8 is a forearm, and board body0 is the
    // motion root. Neither is interchangeable with this physical board pose.
    std::optional<Pose> board_reference;
    skater_body::Contacts body; // what each body touched in the step
};

namespace detail {
// The caller has checked the supported body-transform contract and invokes
// this in the original physics callback. Keep the existing native pool and
// per-body ownership checks together so a replaced holder cannot rebind a pose.
template<class Reader>
std::optional<Step::Pose> read_board_reference_pose(Reader&& read,const NoBailSkater& skater) noexcept {
    namespace state=game::build::v20260929::skater_state;
    namespace body=game::build::v20260929::skater_body;
    namespace source=game::build::v20260929::client_source_spawn;
    std::uintptr_t holder{},physics{},type{},bodies{},owner{};
    std::uint32_t count{};
    if(!read(skater.core+state::board_holder_offset,holder) || !holder ||
        !read(holder+state::board_physics_offset,physics) || !physics ||
        !read(physics,type) || type!=skater.base+state::board_physics_vtable ||
        !read(physics+state::physics_bodies_offset,bodies) || !bodies ||
        !read(bodies,count) || count!=state::board_body_count)return std::nullopt;
    static_assert(source::reference_body_index<state::board_body_count);
    const auto at=bodies+source::reference_body_index*state::physics_body_size;
    Step::Pose pose;
    if(!read(at+state::body_owner_offset,owner) || owner!=physics ||
        !read(at+body::body_transform_offset,pose.transform) ||
        !read(at+state::body_velocity_offset,pose.velocity))return std::nullopt;
    for(float v:pose.velocity)if(!std::isfinite(v) || std::abs(v)>=1000)return std::nullopt;
    for(float v:pose.transform)if(!std::isfinite(v) || std::abs(v)>=100000)return std::nullopt;
    for(unsigned a=0;a<3;++a)for(unsigned b=0;b<=a;++b) {
        float dot{};
        for(unsigned k=0;k<3;++k)dot+=pose.transform[a*4+k]*pose.transform[b*4+k];
        if(std::abs(dot-(a==b?1.f:0.f))>=.05f)return std::nullopt;
    }
    std::uintptr_t after{};std::uint32_t after_count{};
    if(!read(skater.core+state::board_holder_offset,after) || after!=holder ||
        !read(holder+state::board_physics_offset,after) || after!=physics ||
        !read(physics,after) || after!=type ||
        !read(physics+state::physics_bodies_offset,after) || after!=bodies ||
        !read(bodies,after_count) || after_count!=count ||
        !read(at+state::body_owner_offset,after) || after!=physics)return std::nullopt;
    return pose;
}
}

// Requires the validated build and No Bail started: verifies the layout once.
bool start(std::uintptr_t image_base) noexcept;
bool available() noexcept;
// Physics thread, in the step. Exact here; read outside the step, the contacts may be half written.
Step read_step(const NoBailSkater& skater, float seconds, bool wipeout) noexcept;
// The local skater's physics step length (seconds of game time): the game sets it only when it builds
// the skater's core (game_speed.h). 0 while there is no local skater.
float step_length() noexcept;
// Game update thread: gives the local skater this physics step length. False while there is no local
// skater.
bool set_step_length(float seconds) noexcept;
}
