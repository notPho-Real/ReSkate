#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace dingosdk::arcade_mode {
struct NativeGrindControl { bool valid{},neutral{}; };
// Original control-only403 motion and initial402 return admission require the
// transition feature to be off. Existing callers retain the403-only policy.
// This reads flags and raw GrindInputs; it never initializes or completes406.
template<class Read,class Valid>
NativeGrindControl read_grind_control(std::uintptr_t base,std::uintptr_t context,
                                     Read&& read,Valid&& valid,std::uint32_t expected_state=403) noexcept {
    constexpr std::uintptr_t highest=0x00007fffffffffffULL;
    const auto safe=[](std::uintptr_t p){return p>0x10000 && p<highest-0x8000 && !(p&7);};
    if((expected_state!=402 && expected_state!=403) ||
        !safe(base) || base>highest-0x6123098 || !safe(context) || !valid())return {};
    std::uintptr_t flags{},inputs{},table{},again{};
    std::uint32_t state{};std::uint8_t feature{};
    if(!read(context+5140,state) || state!=expected_state ||
        !read(context+5280,flags) || !safe(flags) || !read(flags,table) || table!=base+0x6123098 ||
        !read(flags+26,feature) || feature || !read(context+5248,inputs) ||
        !safe(inputs))return {};
    constexpr std::array<std::uintptr_t,3> offsets{24,60,52};
    std::array<float,3> before{},after{};
    for(unsigned i=0;i<3;++i)if(!read(inputs+offsets[i],before[i]) || !std::isfinite(before[i]))return {};
    for(unsigned i=0;i<3;++i)if(!read(inputs+offsets[i],after[i]))return {};
    if(before!=after || !read(context+5248,again) || again!=inputs ||
        !read(context+5280,again) || again!=flags || !read(flags,table) || table!=base+0x6123098 ||
        !read(flags+26,feature) || feature || !read(context+5140,state) || state!=expected_state || !valid())return {};
    return {true,before[1]==0.f && before[2]==0.f};
}
}
