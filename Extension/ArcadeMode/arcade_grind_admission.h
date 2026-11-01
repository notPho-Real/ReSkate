#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace dingosdk::arcade_mode {
enum class GrindAdmissionControl { ordinary, lateral_tail, lateral_nose, lateral_nose_reverse, lateral_tail_reverse, slide_tail_return };
inline constexpr std::uint64_t grind_lateral_neutral_ms=50;

inline bool grind_admission_source(GrindAdmissionControl control,std::uint32_t state,std::uint32_t identity) noexcept {
    if(control==GrindAdmissionControl::ordinary)return true;
    if(control==GrindAdmissionControl::slide_tail_return)return state==402 && identity==31;
    if(state!=403)return false;
    return ((control==GrindAdmissionControl::lateral_tail || control==GrindAdmissionControl::lateral_tail_reverse) && identity==24) ||
        ((control==GrindAdmissionControl::lateral_nose || control==GrindAdmissionControl::lateral_nose_reverse) && identity==23);
}

// Read the original GrindInputs request, not the smoothed actor orientation.
// Native 403 admission wraps and takes the absolute heading. This guard only
// limits our feature lease; native admission/initialization still selects the
// destination. The qualified lateral receipt is documented in the research note.
template<class Read,class Valid>
bool grind_control_admitted(GrindAdmissionControl control,std::uintptr_t context,
                            std::uint64_t elapsed,Read&& read,Valid&& valid,
                            std::uintptr_t actor=0,std::uintptr_t output=0) noexcept {
    if(control==GrindAdmissionControl::ordinary)return valid();
    const bool slide=control==GrindAdmissionControl::slide_tail_return;
    const bool nose=control==GrindAdmissionControl::lateral_nose || control==GrindAdmissionControl::lateral_nose_reverse;
    if((control!=GrindAdmissionControl::lateral_tail && control!=GrindAdmissionControl::lateral_tail_reverse && !nose && !slide) ||
        elapsed<(slide?200:grind_lateral_neutral_ms) || !context || !valid())return false;
    // The402 receipt requires original actor and published admission together.
    // Valid brackets their native owner/vtable/contact via read_grind_contact;
    // these bytes and the request are observations, never assignments.
    const auto native_admission=[&] {
        if(!slide)return true;
        std::uint8_t active{},published{};
        return actor>0x10000 && actor<0x00007ffffffff000ULL && !(actor&7) &&
            output>0x10000 && output<0x00007ffffffff000ULL && !(output&7) &&
            read(actor+766,active) && active==1 && read(output+419,published) && published==1;
    };
    if(!native_admission())return false;
    // Context+5248 is a pointer, not inline angle storage. Re-read it after
    // sampling so replacing the original GrindInputs object rejects the lease.
    std::uintptr_t inputs{},again{};
    if(!read(context+5248,inputs) || inputs<=0x10000 || inputs>=0x00007fffffffff00ULL || (inputs&7))return false;
    constexpr std::array<std::uintptr_t,3> offsets{24,60,52};
    std::array<float,3> before{},after{};
    for(unsigned i=0;i<3;++i)if(!read(inputs+offsets[i],before[i]) || !std::isfinite(before[i]))return false;
    for(unsigned i=0;i<3;++i)if(!read(inputs+offsets[i],after[i]))return false;
    if(before!=after || !read(context+5248,again) || again!=inputs || !native_admission() || !valid())return false;
    constexpr float pi=std::numbers::pi_v<float>;
    const auto wrapped=std::remainder(before[0],2*pi);
    // Neutral402 keeps its lateral base heading and is not a return request.
    // Only the measured active positive-heading/negative-pitch window admits
    // this lease; original402 admission/initialization still chooses403.
    if(slide)return wrapped>0 && wrapped<35*pi/180 && before[1]>-.5f &&
        before[1]<-.1745329201f && std::abs(before[2])<.001f;
    if((control==GrindAdmissionControl::lateral_nose_reverse || control==GrindAdmissionControl::lateral_tail_reverse) && wrapped>=0)return false;
    const auto heading=std::abs(wrapped);
    const auto pitch=nose?before[1]:-before[1];
    return heading>55*pi/180 && heading<125*pi/180 &&
        pitch>.1745329201f && pitch<.45f && std::abs(before[2])<.001f;
}
}
