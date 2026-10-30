#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

namespace dingosdk::arcade_mode::native_flip {
using Guid = std::array<std::uint8_t, 16>;
// Memory-order GUIDs from the original animation assets, not animation IDs.
inline constexpr Guid pop_guid{0x66,0x22,0x52,0xa1,0xa0,0x61,0xdc,0xb9,0xa1,0x52,0x22,0x66,0x61,0xa0,0xb9,0xdc};
inline constexpr Guid jump_guid{0x5f,0x3f,0xf9,0x9d,0x8f,0xa4,0x03,0x87,0x9d,0xf9,0x3f,0x5f,0xa4,0x8f,0x87,0x03};
struct MotionEntry {
    std::uint32_t key{};
    float tick{};
    float pop_end{};
    std::uintptr_t clip{};
};
inline bool ordinary_flip(std::int32_t id) noexcept {
    switch (id) {
    case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
    case 10: case 11: case 12: case 14: case 15:
    case 17: case 18: case 19: case 20: case 21: case 22: case 23: case 24:
    case 26: case 27: case 28: case 29: case 30:
    case 36: case 37: case 38: case 39: return true;
    default: return false;
    }
}

// The caller owns a newly selected, local TrickLaunch PTM evaluation. Preserve
// its database, selected sequence and mirror bit. Change only the sample used
// by the native motion commit, before it initializes path, pose and actor tags.
// RVA ef2f70 uses these samples to initialize/reuse and release native actors;
// setting the controller's phase afterward is overwritten by its motion path.
template<class Reader>
std::optional<MotionEntry> post_pop_entry(Reader&& read, std::uintptr_t base,
                                        std::uintptr_t result) noexcept {
    auto pointer = [&](std::uintptr_t at, std::uintptr_t& value) {
        if (!read(at,value)) return false;
        value &= ~std::uintptr_t{4};
        return value >= 0x10000 && value < 0x00007fffffff0000ULL;
    };
    auto array = [&](std::uintptr_t at, std::uintptr_t& data, std::uint32_t& count,
                     std::uint32_t maximum) {
        if (!pointer(at,data) || !read(data-4,count)) return false;
        count &= 0x7fffffff;
        return count && count <= maximum;
    };
    std::uint32_t key{}; std::uintptr_t database{}, clips{}, clip{}, vtable{}, sequence{};
    if (!read(result,key) || key==0xffffffff || !pointer(result+8,database) ||
        !pointer(database+0xd0,clips) || !pointer(clips+8*((key>>1)&2047),clip) ||
        !read(clip,vtable) || vtable!=base+0x61620e8 || !pointer(clip+0x28,sequence) ||
        !read(sequence,vtable) || vtable!=base+0x61bab98) return {};
    std::uintptr_t actors{}; std::uint32_t actor_count{};
    if (!array(sequence+0x40,actors,actor_count,4)) return {};
    float end=0; unsigned found=0;
    for (std::uint32_t a=0;a<actor_count;++a) {
        std::uintptr_t actor{}, set{}, collections{}; std::uint32_t collection_count{};
        if (!pointer(actors+8*a,actor) || !read(actor,vtable) || vtable!=base+0x61ba838 ||
            !pointer(actor+0x30,set) || !array(set+0x28,collections,collection_count,16)) return {};
        for (std::uint32_t c=0;c<collection_count;++c) {
            std::uintptr_t collection{}, tags{}; std::uint32_t tag_count{};
            if (!pointer(collections+8*c,collection) || !array(collection+0x28,tags,tag_count,128)) return {};
            for (std::uint32_t t=0;t<tag_count;++t) {
                std::uintptr_t tag{}, state{}; Guid guid{};
                if (!pointer(tags+8*t,tag) || !read(tag,vtable)) return {};
                if (vtable!=base+0x6189e38) continue; // BoolGameStateTagAsset
                if (!pointer(tag+0x40,state)) continue; // Unbound tag.
                if (!read(state-16,guid)) return {};
                const unsigned bit=guid==pop_guid ? 1u : guid==jump_guid ? 2u : 0u;
                if (!bit) continue;
                float start{}, duration{}; std::uint16_t flags{};
                std::uint32_t value{}; std::uintptr_t enable{};
                if (!read(tag+0x28,start) || !read(tag+0x30,duration) || !read(tag+0x2c,flags) ||
                    !read(tag+0x38,enable) || !read(tag+0x50,value) || enable || value!=1 ||
                    (flags&0xc0) || !std::isfinite(start) || !std::isfinite(duration) ||
                    start<0 || start>=60 || duration<=0 || duration>=15) return {};
                // Native tag helpers 11f6410/11f6250: unscaled start and duration.
                if (start+duration>end) end=start+duration;
                found|=bit;
            }
        }
    }
    if (found!=3) return {};
    std::uintptr_t samples{}; std::uint32_t count{};
    if (!array(clip+0x38,samples,count,1024)) return {};
    const auto selected=(key>>12)&16383;
    float selected_tick{};
    if (selected>=count || !read(samples+16*selected+4,selected_tick) ||
        !std::isfinite(selected_tick)) return {};
    if (selected_tick>end) return {}; // Native selection already passed pop.
    float previous=-1;
    for (std::uint32_t i=0;i<count;++i) {
        float tick{};
        if (!read(samples+16*i+4,tick) || !std::isfinite(tick) || tick<previous) return {};
        previous=tick;
        // The ending tick is inclusive: starting at end re-fires Pop/Jump.
        if (tick>end) {
            if (tick>end+4) return {};
            return MotionEntry{(key&~0x03fff000u)|(i<<12),tick,end,clip};
        }
    }
    return {};
}
}
