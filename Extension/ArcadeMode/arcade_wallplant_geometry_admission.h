#pragma once
#include "arcade_native_input_state.h"

namespace dingosdk::arcade_mode {
struct WallplantSurfaceFrame {
    NativeInputFrame frame{};
    bool wallplant_only{};
};
// A retained contact uses synchronized validation for the entire query, even
// when the first frame read succeeded before a publisher acquired the lock.
// Ordinary geometry remains nonblocking. No timestamp is rewritten.
template<class Synchronize,class Revalidate>
WallplantSurfaceFrame wallplant_surface_frame(const NativeInputFrame& ordinary,bool retained,
        Synchronize&& synchronize,Revalidate&& revalidate) {
    if(!retained)return {ordinary,false};
    const auto frame=ordinary.valid?ordinary:synchronize();
    return frame.valid && revalidate(frame) ? WallplantSurfaceFrame{frame,true} : WallplantSurfaceFrame{};
}
}
