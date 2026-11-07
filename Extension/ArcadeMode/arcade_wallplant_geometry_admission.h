#pragma once
#include "arcade_native_input_state.h"

namespace dingosdk::arcade_mode {
struct WallplantSurfaceFrame {
    NativeInputFrame frame{};
    bool wallplant_only{};
};
// Ordinary geometry retains its original nonblocking result. Only an actual
// retained contact may acquire a separate synchronized frame; its caller must
// revalidate the same contact after acquisition. No timestamp is rewritten.
template<class Synchronize,class Revalidate>
WallplantSurfaceFrame wallplant_surface_frame(const NativeInputFrame& ordinary,bool retained,
        Synchronize&& synchronize,Revalidate&& revalidate) {
    if(ordinary.valid)return {ordinary,false};
    if(!retained)return {};
    const auto frame=synchronize();
    return frame.valid && revalidate(frame) ? WallplantSurfaceFrame{frame,true} : WallplantSurfaceFrame{};
}
}
