#include "Extension/ArcadeMode/arcade_gameplay_profile.h"
#include "Extension/ArcadeMode/arcade_movement.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void close(float actual, float expected, const char* why) {
    ++checks;
    if (!std::isfinite(actual) || std::abs(actual - expected) > .00002f) {
        std::cerr << "FAIL: " << why << " actual=" << actual << " expected=" << expected << '\n'; std::exit(1);
    }
}
}
int main() {
    GameplayProfile ten, five{GameplayStats::standard};
    close(ten.effective_stat(), 10, "default selected profile is10/10");
    close(ten.kick_speed(false), 12.5984f, "standing10 kick threshold");
    close(ten.kick_speed(true), 17.145f, "crouched10 kick threshold");
    close(ten.kick_acceleration(false), 17.78f, "standing10 acceleration");
    close(ten.kick_acceleration(true), 30.48f, "crouched10 acceleration");
    close(five.kick_speed(false), 11.303f, "standing5 kick threshold");
    close(five.kick_acceleration(true), 28.6639f, "crouched5 acceleration");
    close(ten.soft_speed(), 22.86f, "soft speed is distinct from kick threshold");
    close(ten.hard_speed(), 27.94f, "hard speed is distinct from soft speed");
    close(ten.angular_speed(), 7.75f, "maxed angular rate");
    close(five.angular_speed(), 7.3f, "standard angular rate");
    five.switched = true;
    close(five.kick_speed(false), 10.1727f, "kick switch factor is separate from standard factor");
    close(five.angular_speed(), 6.935f, "standard switch factor applies to spin");
    ten.switched = true;
    close(ten.kick_speed(false), 12.5984f, "maxed switch factor reaches one");
    ten.special = true;
    close(ten.angular_speed(), 8.02f, "modifier remains an explicit configuration input");
    close(ten.flip_speed(1), 1.3f, "default action cap still binds");
    close(ten.flip_speed(1, 1.5f), 1.342f, "explicit action cap can differ");
    ten = {};
    close(ten.flip_speed(.8f), 1.04f, "authored action speed is multiplied before its cap");
    close(ten.flip_speed(1, 1.1f), 1.1f, "lower per-action cap binds at10");
    close(ten.jump_speed(0), 8.89f, "flat jump minimum remains fixed");
    close(ten.jump_speed(100), 10.16f, "charge midpoint");
    close(ten.jump_speed(200), 11.43f, "flat charged maximum");
    close(ten.jump_speed(250), 11.43f, "charge saturates at200ms");
    close(ten.jump_speed(200, true), 6.985f, "ordinary vert maximum remains fixed");
    close(ten.jump_speed(200, true, true), 8.255f, "boneless vert uses its own family");
    close(ten.jump_speed(200, false, true), 13.335f, "flat boneless uses its own family");
    for (unsigned charge = 0; charge <= 250; ++charge)
        close(ten.jump_speed(charge), 8.89f + 2.54f * static_cast<float>(std::min(charge, 200u)) / 200,
              "complete charge sweep follows selected physical envelope");
    const auto manual = ten.balance(PhysicalBalanceKind::manual);
    const auto grind = ten.balance(PhysicalBalanceKind::grind);
    const auto lip = ten.balance(PhysicalBalanceKind::lip);
    close(manual.instability_rate, .07f, "descending manual instability coefficient");
    close(manual.cheese, 700, "equal endpoint parameter stays fixed");
    close(grind.repeat_multiplier, .1f, "descending grind repeat coefficient");
    close(grind.new_rail_subtraction, 0, "maxed new rail subtraction");
    close(lip.cheese, 1000, "descending lip parameter");
    five.switched = false;
    close(five.balance(PhysicalBalanceKind::grind).new_rail_subtraction, -.143f, "standard new rail subtraction retains its sign");
    ten.special = true;
    close(ten.balance(PhysicalBalanceKind::grind).random_speed, 6, "descending random floor binds before integer conversion");
    ten = {}; ten.difficulty = GameplayDifficulty::easy;
    close(ten.balance(PhysicalBalanceKind::manual).gravity, .00002f, "easy gravity multiplier");
    close(ten.balance(PhysicalBalanceKind::manual).instability_rate, .035f, "easy manual instability multiplier");
    ten.difficulty = GameplayDifficulty::hard;
    close(ten.balance(PhysicalBalanceKind::manual).instability_rate, .14f, "hard manual instability multiplier");
    close(ten.balance(PhysicalBalanceKind::grind).instability_rate, .09f, "manual difficulty rate does not leak to rail");
    GroundDrive standard(five);
    close(standard.step(.017f, {}, {}, false, true), 16.8783f * .017f, "existing body writer receives selected5 acceleration");
    standard.reset();
    close(standard.step(.017f, {}, {}, false, true), 16.8783f * .017f, "reset preserves the chosen profile");
    GroundDrive maxed;
    close(maxed.step(.017f, {}, {}, false, true), 17.78f * .017f, "production default uses selected10 acceleration");
    std::cout << checks << " gameplay profile checks passed\n";
}
