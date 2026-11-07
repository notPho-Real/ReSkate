#pragma once
#include "Engine/Game/Build/20260929/animation_values.h"
#include <array>
#include <cstdint>

namespace dingosdk {
struct NativeOrientation {
    bool valid{}, fakie{};
    bool operator==(const NativeOrientation&) const = default;
};
// The same corroborated native stance sources as plugin skater_frame(). This
// is observation only; neither a leading-foot bit nor an input request is fakie.
template<class Reader>
NativeOrientation read_native_orientation(Reader&& read, std::uintptr_t core,
                                         std::uintptr_t context) noexcept {
    namespace layout = game::build::v20260929::animation_values;
    const auto pointer=[](std::uintptr_t p) { return p>=0x10000 && p<0x00007fffffff0000ULL && !(p&7); };
    std::uintptr_t source{},root{},exported{};
    std::array<std::uint8_t,5> flags{};
    std::array<std::uint8_t,3> exports{};
    std::uint32_t native{},basis{};
    if (!pointer(core) || !pointer(context) || !read(core+layout::frame_source,source) || !pointer(source) ||
        !read(source+layout::frame_flags,flags) || flags[0]>1 || flags[2]>1 || flags[3]>1 || flags[4]>1 ||
        !read(context+layout::frame_context,native) ||
        (flags[4]!=0)!=((native&layout::native_mask)!=0) ||
        !read(context+layout::basis_context,basis) || (flags[0]!=0)!=((basis&layout::basis_mask)!=0) ||
        !read(core+layout::state_root,root) || !pointer(root) ||
        !read(root+layout::export_state,exported) || !pointer(exported) ||
        !read(exported+layout::exported_flags,exports) || exports[0]!=flags[4] ||
        exports[1]!=flags[2] || exports[2]!=flags[3]) return {};
    // The native frame and export can both publish the two adjacent flags as
    // true while riding. They are not an exclusive stance enumeration. Fakie
    // is the separate flag corroborated by the physics word and export above.
    return {true,flags[4]!=0};
}
}
