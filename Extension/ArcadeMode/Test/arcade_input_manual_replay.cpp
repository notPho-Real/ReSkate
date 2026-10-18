#include "Extension/ArcadeMode/Test/ReferenceInput/arcade_manual_input.h"
#include "Extension/ArcadeMode/Test/session_manual.h"
#include <iostream>
#include <random>
using namespace dingosdk::arcade_mode;
int main() {
    std::mt19937 rng(0x51a7u);
    unsigned comparisons{}, diagnostic_differences{};
    for (unsigned trace = 0; trace < 1000; ++trace) {
        dingosdk::arcade_mode::test::SessionManual replacement; baseline::ManualGesture baseline;
        std::uint64_t now{};
        for (unsigned frame = 0; frame < 200; ++frame) {
            now += trace % 2 ? 17 : 34;
            const auto choice = rng() % 12;
            const std::uint16_t buttons = choice == 0 ? 0x2000 : choice == 1 ? 1 : choice == 2 ? 2 : choice == 3 ? 0x1000 : 0;
            const auto x = static_cast<std::int16_t>((static_cast<int>(rng() % 7) - 3) * 8000);
            const auto y = static_cast<std::int16_t>((static_cast<int>(rng() % 7) - 3) * 8000);
            const auto pose = trace % 7 == 0 ? SkatePose::air : SkatePose::riding;
            const bool safe = rng() % 8 != 0, conflict = rng() % 15 == 0, gameplay = rng() % 99 != 0;
            const auto a = replacement.poll(now, buttons, x, y, gameplay, safe, pose, conflict);
            const auto b = baseline.poll(now, buttons, x, y, gameplay, safe, pose, conflict);
            const auto da = replacement.diagnostic(); const auto db = baseline.diagnostic();
            if (da.stick_first != db.stick_first || da.pad_first != db.pad_first ||
                da.stick_at != db.stick_at || da.pad_at != db.pad_at) ++diagnostic_differences;
            if (a.owns_stick != b.owns_stick || a.y != b.y || a.confirmed != b.confirmed ||
                a.suppress_reverse != b.suppress_reverse || da.stick_first != db.stick_first ||
                da.pad_first != db.pad_first || da.stick_at != db.stick_at || da.pad_at != db.pad_at) {
                std::cerr << "values " << a.owns_stick << "," << b.owns_stick << " first " << da.stick_first << "," << db.stick_first << " at " << da.stick_at << "," << db.stick_at << " pad " << da.pad_first << "," << db.pad_first << " pat " << da.pad_at << "," << db.pad_at << " input " << buttons << "," << x << "," << y << " safe " << safe << " conf " << conflict << "\n";
                std::cerr << "differential mismatch trace " << trace << " frame " << frame << '\n'; return 1;
            }
            ++comparisons;
        }
    }
    std::cout << comparisons << " identical fixed-seed output samples; " << diagnostic_differences << " journal-view diagnostic differences\n";
}
