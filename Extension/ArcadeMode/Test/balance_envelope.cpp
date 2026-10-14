// Standalone deterministic tuning experiment, no game or controller dependency.
#include "Extension/ArcadeMode/arcade_model.h"
#include <iostream>
using namespace dingosdk::arcade_mode;
int main() {
    std::cout << "{\"schema\":1,\"evidence\":\"SIMULATED\",\"runs\":[";
    bool first = true;
    for (int age : {0, 10, 30}) for (int hz : {30, 60, 120}) for (int control : {0, 1, 2}) {
        Model m({}, 42);
        m.trick(100);
        for (int i = 0; i < age; ++i) m.tick(1);
        m.begin_balance(BalanceKind::grind);
        const float initial = m.difficulty();
        int steps = 0;
        float maximum = 0;
        for (; steps < hz * 60 && m.state().combo_active; ++steps) {
            // 0 neutral, 1 continuous wrong-way, 2 simple feedback controller.
            const auto& s = m.state();
            const float u = control == 0 ? 0.0f : control == 1 ? 1.0f :
                std::clamp(-2.0f * s.balance - s.balance_velocity, -1.0f, 1.0f);
            maximum = std::max(maximum, std::abs(s.balance));
            m.tick(1.0f / static_cast<float>(hz), u);
        }
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"age_s\":" << age << ",\"hz\":" << hz << ",\"control\":" << control
            << ",\"initial_difficulty\":" << initial << ",\"elapsed_s\":"
            << static_cast<float>(steps) / static_cast<float>(hz)
            << ",\"failed\":" << (m.state().combo_active ? "false" : "true")
            << ",\"max_abs_balance\":" << maximum << '}';
    }
    std::cout << "]}\n";
}
