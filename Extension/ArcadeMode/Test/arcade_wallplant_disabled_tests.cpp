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
        std::uint64_t now=10;
        bool stale=true;
        if(wallplant_runtime::begin_motion(frame,now) || wallplant_runtime::motion_current(1,frame) ||
           wallplant_runtime::retain_collision_before_response(frame,now) ||
           wallplant_runtime::begin_hold(command,now) || wallplant_runtime::hold_current(command,now,frame,&stale) ||
           stale || now!=10 || wallplant_runtime::held_command() || wallplant_runtime::hold_started_at() ||
           wallplant_runtime::preserves_combo(frame,now) || wallplant_runtime::excludes_grind(frame,now) ||
           wallplant_runtime::writer_probe().input || wallplant_runtime::retained_writer_probe().contact ||
           wallplant_runtime::geometry_probe().contact || wallplant_runtime::geometry_probe_current({},frame))
            return EXIT_FAILURE;
        wallplant_runtime::end_motion(1);
        wallplant_runtime::finish_hold(command,true,now);
        wallplant_runtime::motion_diagnostic({});
        wallplant_runtime::writer_event({},1,"disabled",100,true,1);
        wallplant_runtime::retained_writer_event({},1,"disabled",100,true,1);
        wallplant_runtime::geometry_event({},1,"disabled");
        wallplant_runtime::completed(command, true);
        if (wallplant_runtime::latest_completed()) return EXIT_FAILURE;
        wallplant_runtime::interrupt_animation();
        wallplant_runtime::cancel();
    }
    std::cout << "Disabled wallplants have no native dependencies, permissions or receipts\n";
}
