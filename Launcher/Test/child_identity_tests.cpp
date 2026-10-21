#include "Launcher/child_identity.h"
#include <iostream>
#include <functional>
using namespace dingosdk::launcher_app::identity;
int main() {
    unsigned checks = 0;
    auto check = [&](bool value) { ++checks; if (!value) throw std::runtime_error("Identity regression failed"); };
    const Observation first{7, 100000001, 200000000, 1000, 0, true, true, true};
    auto last = first; last.observed += 100000; last.tick += 10;
    validate(first, last, 7, 100000001); check(true);
    auto reject = [&](Observation altered) {
        bool rejected = false;
        try { validate(first, altered, 7, 100000001); } catch (const std::runtime_error&) { rejected = true; }
        check(rejected);
    };
    for (unsigned field = 0; field < 12; ++field) {
        auto altered = last;
        switch (field) {
        case 0: altered.pid = 8; break;
        case 1: altered.created += 1; break; // Same-second PID reuse is different.
        case 2: altered.alive = false; break;
        case 3: altered.image_matches = false; break;
        case 4: altered.module_matches = false; break;
        case 5: altered.observed = first.observed - 1; break;
        case 6: altered.tick = first.tick - 1; break;
        case 7: altered.observed += 50000001; break;
        case 8: altered.tick += 5001; break;
        case 9: altered.utc_offset_seconds += 3600; break;
        case 10: altered.observed += 2000000; break;
        default: altered.tick += 200; break;
        }
        reject(altered);
    }
    const auto output = response(first, last, std::string(64, 'b'), std::string(64, 'c'), std::string(32, 'd'));
    check(output.size() < 2048); check(output.back() == '\n');
    check(output.find("100000001") != std::string::npos);
    check(output.find("UNVERIFIED") != std::string::npos);
    for (const auto& invalid : {std::string(), std::string(63, 'b'), std::string(64, 'G'), std::string(65, 'b')}) {
        bool rejected = false;
        try { (void)response(first, last, invalid, std::string(64, 'c'), std::string(32, 'd')); }
        catch (const std::runtime_error&) { rejected = true; }
        check(rejected);
    }
    std::cout << checks << " child identity checks passed\n";
}
