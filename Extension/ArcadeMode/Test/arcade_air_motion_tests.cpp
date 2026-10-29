#include "Extension/ArcadeMode/arcade_air_motion.h"
#include "Extension/ArcadeMode/arcade_rotation.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value, const char* why) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
}
bool near(float a, float b) { return std::abs(a - b) < .0001f; }
struct Fixture {
    AirMotion controller;
    AirMotionIntent intent{true, true, 1000, 0, 0, 0, 0, 1, {}, {}, {}, LaunchDescriptor::ordinary()};
    AirMotionFrame frame{1000, 1, 100, 1.0f / 60, 5, 2, 0, true};
    AirMotionOutput sample(std::uint64_t now, unsigned state) {
        intent.sampled_at = frame.now = now; frame.state = state;
        const auto output=controller.step(intent,frame);
        if(output.vertical) frame.velocity_y=output.velocity_y; // The production writer delivers this root velocity.
        return output;
    }
};
}
int main() {
    for(unsigned hz:{30u,60u,120u})for(unsigned delay:{0u,17u,33u,66u,100u}) {
        NativeCharge charge;charge.admit_setup(1000,1000,LaunchDescriptor::ordinary(),true);
        auto released=charge.semantic_release(1100,true,true,false,1000,1100,
            NativeCharge::input_permitted(true,false,delay==0?0x4000u:0u));
        check(released.sequence!=0,"simultaneous A release and X retain launch identity");
        for(unsigned n=0;n<20;++n){const auto now=1100+n*1000/hz;
            const auto next=charge.semantic_release(now,false,false,false,0,0,
                NativeCharge::input_permitted(true,false,now>=1100+delay?0x4000u:0u));
            check(next.sequence==released.sequence&&next.ms==1100&&next.charge_ms==100,
                "following X cannot erase or renew original charged launch before physics consumes it");
        }
        check(!charge.semantic_release(1800,false,false,false,0,0,NativeCharge::input_permitted(true,false,0x8000)).sequence,
            "incompatible rail input still withdraws ordinary launch");
    }

    NativeCharge charge;
    check(!charge.poll(1000, false, true, true).sequence && !charge.poll(1017, false, false, true).sequence,
          "raw A without accepted native crouch can never create a launch token");
    charge.poll(1100, true, true, true);
    const auto short_release = charge.poll(1200, false, false, true);
    check(short_release.sequence == 1 && short_release.charge_ms == 100 && short_release.ms == 1200,
          "accepted native crouch owns measured charge until physical release");
    check(charge.poll(1217, false, false, true).sequence == 1, "repeated release does not create another launch");
    charge.poll(1300, true, true, true);
    check(charge.poll(1800, false, false, true).charge_ms == 200, "charge saturates without delaying physical release");
    charge.poll(1900, true, true, true); charge.poll(1917, false, true, false);
    check(!charge.poll(1934, false, false, true).sequence, "lost native execution ownership cancels acquisition");
    for (unsigned duration : {0u, 1u, 100u, 199u, 200u, 250u}) {
        Fixture f;
        check(f.sample(1000, 100).ready, "validated ordinary ground establishes the native controller owner");
        f.intent.release_sequence = 1; f.intent.release_at = 1017; f.intent.charge_ms = duration;
        f.sample(1017, 103);
        f.frame.velocity_y = 30; // Native jump already applied an impulse.
        const auto launch = f.sample(1034, 200);
        const auto expected = 2 + GameplayProfile{}.launch_speed(duration,LaunchDescriptor::ordinary());
        check(launch.vertical && near(launch.velocity_y, expected) && launch.velocity_y < 30,
              "takeoff compensates to the charged target instead of adding a second impulse");
        const auto next = f.sample(1051, 201);
        check(near(next.velocity_y, expected - 34.29f / 60), "actual simulation step integrates selected flat-air gravity");
        f.intent.release_at = 1051; // A reread cannot renew a consumed identity.
        check(f.sample(1068, 201).velocity_y < next.velocity_y, "same launch token cannot relaunch or renew gravity history");
        check(!f.sample(1085, 100).vertical, "native contact ends controlled air motion without forcing state");
        f.frame.velocity_y=30;
        check(near(f.sample(1102, 200).velocity_y, 30),
              "a later uncommanded air episode does not reuse the old launch token");
    }
    Fixture expired; expired.sample(1000, 100);
    expired.intent.release_sequence = 1; expired.intent.release_at = 1017;
    for (std::uint64_t now = 1017; now <= 1204; now += 17) expired.sample(now, 100);
    check(near(expired.sample(1221, 200).velocity_y, 2), "repeated candidate publications cannot renew the execution deadline");
    Fixture falling; falling.frame.velocity_y = -2; falling.sample(1000, 100);
    falling.intent.release_sequence = 1; falling.intent.release_at = 1017; falling.sample(1017, 103);
    falling.frame.velocity_y = 30;
    check(near(falling.sample(1034, 200).velocity_y, 15.7353f), "negative prelaunch vertical velocity is cleared rather than deducted from selected jump");
    for (unsigned fault = 0; fault < 9; ++fault) {
        Fixture f; f.sample(1000, 100); f.intent.release_sequence = 1; f.intent.release_at = 1017;
        f.sample(1017, 103);
        f.frame.now = 1034; f.intent.sampled_at = 1034; f.frame.state = 200;
        if (fault == 0) f.intent.enabled = false;
        if (fault == 1) f.frame.owner = 2;
        if (fault == 2) f.intent.sampled_at = 800;
        if (fault == 3) f.frame.dt = 0;
        if (fault == 4) f.frame.now = f.intent.sampled_at = 1000;
        if (fault == 5) f.frame.state = 300;
        if (fault == 6) f.intent.generation = 2;
        if (fault == 7) f.frame.dt = .2f;
        if (fault == 8) f.intent.yaw_rate = 100;
        check(!f.controller.step(f.intent, f.frame).vertical,
              "OFF, owner loss, stale input, invalid cadence, rollback, bail and generation changes cancel native launch ownership");
    }
    Fixture f; f.frame.ordinary_ground = false;
    check(!f.sample(1000, 100).ready && !f.sample(1017, 200).vertical,
          "unknown surface does not silently select the ordinary flat-air controller");
    Fixture spin; spin.sample(1000, 100); spin.intent.yaw_rate = 7.75f;
    auto turned = spin.sample(1017, 200);
    check(turned.angular && near(turned.spin_y, 7.75f) && near(turned.yaw, 7.75f / 60),
          "supported shoulder intent executes selected angular rate per simulation step");
    spin.intent.yaw_rate = 0;
    const auto released = spin.sample(1034, 201);
    check(released.angular && !released.spin_y && near(released.yaw, turned.yaw),
          "release stops additional angular rate rather than prescribing a fixed turn duration");
    check(!spin.sample(1051, 201).angular, "native angular ownership relinquishes after the release step");
    std::array<float, 16> matrix{1,0,0,0, 0,1,0,0, 0,0,1,0, 4,5,6,1};
    AirMotion::rotate_target(matrix, 1.57079632679f);
    check(near(matrix[8], 1) && near(matrix[10], 0) && matrix[12] == 4 && matrix[13] == 5 && matrix[14] == 6,
          "native orientation composition preserves the position and rotates the heading basis");
    AirRotation shoulder;
    shoulder.poll(1000, 0, true, SkatePose::air, false, false, false, true);
    shoulder.poll(1017, BoardChord::rb, true, SkatePose::air, false, false, false, true);
    const auto held_rate = shoulder.poll(1140, BoardChord::rb, true, SkatePose::air, false, false, false, true);
    check(held_rate.owns_left && held_rate.x == 0 && held_rate.y == 0 && held_rate.yaw_rate == 7.75f,
          "native rate control replaces its competing LS spin recipe while retaining chord arbitration");
    for (unsigned rate : {30u, 60u}) {
        Fixture cadence; cadence.frame.dt = 1.0f / static_cast<float>(rate); cadence.sample(1000, 100);
        cadence.intent.yaw_rate = 7.75f;
        AirMotionOutput integrated{};
        for (unsigned i = 1; i <= rate; ++i) integrated = cadence.sample(1000 + i * (rate == 30 ? 34u : 17u), 200);
        check(near(integrated.yaw, std::remainder(7.75f, 6.28318530718f)), "angular execution uses simulation seconds rather than callback count");
        cadence.frame.now += 200; cadence.intent.sampled_at = cadence.frame.now;
        check(!cadence.controller.step(cadence.intent, cadence.frame).ready, "a missed physics interval cannot resume an old controlled air episode");
    }
    std::array<std::array<float, 3>, 32> velocities{}, spins{};
    for (unsigned i = 0; i < 32; ++i) {
        velocities[i] = {3, 30 + static_cast<float>(i) * .1f, -4};
        spins[i] = {.5f, 2 + static_cast<float>(i) * .01f, -.5f};
    }
    const auto original_velocities = velocities, original_spins = spins;
    check(AirMotion::apply_bodies(AirMotionOutput{true, true, true, 9, 7.75f, 0}, velocities, spins),
          "validated native body batch accepts selected root motion");
    for (unsigned i = 0; i < 32; ++i) {
        check(velocities[i][0] == 3 && velocities[i][2] == -4 && spins[i][0] == .5f && spins[i][2] == -.5f &&
              near(velocities[i][1], original_velocities[i][1] - 21) && near(spins[i][1], original_spins[i][1] + 5.75f),
              "body actuation preserves horizontal and articulated relative motion");
    }
    const auto moved_velocities = velocities;
    check(AirMotion::apply_bodies(AirMotionOutput{true, false, true, 0, 0, 0}, velocities, spins) && velocities == moved_velocities,
          "angular release never changes linear velocity");
    velocities = original_velocities; spins = original_spins;
    velocities[31][1] = 999;
    const auto invalid_velocities = velocities;
    check(!AirMotion::apply_bodies(AirMotionOutput{true, true, true, 999, 7.75f, 0}, velocities, spins) &&
          velocities == invalid_velocities && spins == original_spins,
          "an invalid final body rejects the entire batch before modifying any body");
    velocities = original_velocities; spins = original_spins;
    check(!AirMotion::apply_bodies(AirMotionOutput{}, velocities, spins) && velocities == original_velocities && spins == original_spins,
          "unready execution cannot mutate a body batch");
    for (float pitch : {-.5235987756f,.5235987756f,3.14159265359f}) for (float yaw : {-1.57079632679f,1.57079632679f}) {
        const float c=std::cos(pitch), s=std::sin(pitch);
        std::array<float,16> tilted{1,0,0,0, 0,c,s,0, 0,-s,c,0, 4,5,6,1};
        const auto original=tilted;
        AirMotion::rotate_target(tilted,yaw);
        check(tilted[4]==original[4]&&tilted[5]==original[5]&&tilted[6]==original[6],
              "local-Y rotation preserves a pitched or inverted up basis exactly");
        check(near(tilted[8],std::sin(yaw))&&near(tilted[9],-std::cos(yaw)*s)&&near(tilted[10],std::cos(yaw)*c),
              "rotated heading follows local frame composition instead of world-Y motion");
        AirMotion::rotate_target(tilted,-yaw);
        for(unsigned i=0;i<16;++i) check(near(tilted[i],original[i]),"opposite local turn restores the full native transform");
        velocities=original_velocities; spins=original_spins;
        const std::array<float,3> up{0,c,s};
        check(AirMotion::apply_bodies(AirMotionOutput{true,false,true,0,7.75f,0},velocities,spins,up),
              "local angular target reaches the physical body batch");
        check(near(spins[0][1]*c+spins[0][2]*s,7.75f)&&near(spins[0][1]*(-s)+spins[0][2]*c,
              original_spins[0][1]*(-s)+original_spins[0][2]*c)&&velocities==original_velocities,
              "physical angular correction selects local-up rate while preserving perpendicular spin and linear motion");
    }
    for(float impulse : {-5.0f,4.0f}) {
        Fixture external;external.sample(1000,100);
        const auto launched=external.sample(1017,200);
        external.frame.velocity_y=launched.velocity_y+impulse;
        external.intent.yaw_rate=7.75f;
        const auto changed=external.sample(1034,201);
        check(!changed.vertical&&changed.angular&&near(changed.velocity_y,launched.velocity_y+impulse),
              "a changed native vertical impulse relinquishes cached gravity while preserving local angular ownership");
        check(!external.sample(1051,201).vertical,"same-air native takeover cannot silently restart cached ballistic ownership");
        external.sample(1068,100);
        check(external.sample(1085,200).vertical,"a new independently grounded air episode can reacquire ordinary gravity");
    }
    Fixture integrated;integrated.sample(1000,100);
    const auto first_air=integrated.sample(1017,200);
    integrated.frame.velocity_y=first_air.velocity_y-34.29f/60;
    check(near(integrated.sample(1034,201).velocity_y,first_air.velocity_y-34.29f/60),
          "already integrated selected native gravity is credited rather than applied twice");
    std::cout << checks << " native air motion execution checks passed\n";
}
