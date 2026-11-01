#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace dingosdk::arcade_mode {
enum class GrindAdmissionControl { ordinary, lateral_tail };
inline constexpr std::uint64_t grind_lateral_neutral_ms=50;

// Read the original GrindInputs request, not the smoothed actor orientation.
// Native 403 admission wraps and takes the absolute heading. This guard only
// limits our feature lease; native admission/initialization still selects the
// destination. The qualified lateral receipt is documented in the research note.
template<class Read,class Valid>
bool grind_control_admitted(GrindAdmissionControl control,std::uintptr_t context,
                            std::uint64_t elapsed,Read&& read,Valid&& valid) noexcept {
    if(control==GrindAdmissionControl::ordinary)return valid();
    if(control!=GrindAdmissionControl::lateral_tail || elapsed<grind_lateral_neutral_ms || !context || !valid())return false;
    // Context+5248 is a pointer, not inline angle storage. Re-read it after
    // sampling so replacing the original GrindInputs object rejects the lease.
    std::uintptr_t inputs{},again{};
    if(!read(context+5248,inputs) || inputs<=0x10000 || inputs>=0x00007fffffffff00ULL || (inputs&7))return false;
    constexpr std::array<std::uintptr_t,3> offsets{24,60,52};
    std::array<float,3> before{},after{};
    for(unsigned i=0;i<3;++i)if(!read(inputs+offsets[i],before[i]) || !std::isfinite(before[i]))return false;
    for(unsigned i=0;i<3;++i)if(!read(inputs+offsets[i],after[i]))return false;
    if(before!=after || !read(context+5248,again) || again!=inputs || !valid())return false;
    constexpr float pi=std::numbers::pi_v<float>;
    const auto heading=std::abs(std::remainder(before[0],2*pi));
    return heading>55*pi/180 && heading<125*pi/180 &&
        before[1]>-.45f && before[1]<-.1745329201f && std::abs(before[2])<.001f;
}
}
