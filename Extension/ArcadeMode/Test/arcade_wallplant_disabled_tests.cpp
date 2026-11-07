#include "../arcade_wallplant_runtime.h"
#include <cstdlib>
#include <iostream>

using namespace dingosdk::arcade_mode;
int main() {
    // Link the real disabled runtime without Windows/native implementations.
    // Any accidental hook, animation or body-adapter dependency fails this link.
    NativeInputFrame frame{};
    frame.valid = true;
    frame.airborne = true;
    InputRequest press{};
    press.action = InputAction::wall_command;
    press.variant = 1;
    press.input_sequence = 42;
    WallplantCommand command{};
    for (bool gameplay : {false, true, false, true}) {
        wallplant_runtime::tick(1, gameplay);
        wallplant_runtime::observe_physics(frame);
        const auto surface = wallplant_runtime::surface(10, frame);
        if (surface.fresh || surface.can_plant || surface.can_push || surface.can_transfer ||
            wallplant_runtime::request(press, 10, frame) != InputDisposition::unsupported ||
            wallplant_runtime::consume(10, frame, {0, 0, 10}) ||
            wallplant_runtime::current(command, 10, frame, {0, 0, 10})) return EXIT_FAILURE;
        wallplant_runtime::completed(command, true);
        if (wallplant_runtime::latest_completed()) return EXIT_FAILURE;
        wallplant_runtime::interrupt_animation();
        wallplant_runtime::cancel();
    }
    std::cout << "Disabled wallplants have no native dependencies, permissions or receipts\n";
}
