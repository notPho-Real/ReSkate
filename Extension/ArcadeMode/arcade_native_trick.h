#pragma once
#include "Engine/Game/Build/20260929/arcade_tricks.h"
#include <cstdint>

namespace dingosdk::arcade_mode {
struct NativeFlipSelection {
    bool valid{};
    std::int32_t id{-1};
    bool operator==(const NativeFlipSelection&) const = default;
};
// Reader is a bounded read(address, typed_value). Owner validation is the caller's
// responsibility; a replaced selection object invalidates this read as well.
template<class Reader>
NativeFlipSelection read_flip_selection(Reader&& read, std::uintptr_t core) noexcept {
    namespace layout = game::build::v20260929::arcade_tricks;
    constexpr std::uintptr_t highest = 0x00007fffffffffffULL;
    if (core < 0x10000 || core > highest - layout::core_selection) return {};
    std::uintptr_t before{}, after{};
    std::int32_t id{-1};
    if (!read(core + layout::core_selection, before) || before < 0x10000 || before > highest - layout::selection_flip ||
        !read(before + layout::selection_flip, id) || id < -1 || id > 63 ||
        !read(core + layout::core_selection, after) || after != before) return {};
    return {true, id};
}
}
