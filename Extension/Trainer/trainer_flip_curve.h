#pragma once
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace dingosdk::trainer::flip_curve_detail {
// The existing resolved speed curves have two or three points. This helper
// neither discovers assets nor identifies a skater, trick or animation stage.
inline constexpr std::size_t max_points = 3, max_fields = max_points + 2;
inline constexpr std::uintptr_t point_stride = 0x1c, point_x = 0xc, point_y = 0x14, points_at = 0x18;
enum class Identity { same, unavailable, replaced };
enum class Status { unchanged, applied, unavailable, restore_pending, retired };
struct Result { Status status{}; std::size_t outputs_written{}; };
struct Curve {
    std::uintptr_t curve{}, points{}, type{};
    std::uint32_t count_word{}, count{};
    std::array<float,max_points> inputs{}, outputs{};
    float low{}, high{}, factor{1};
    std::array<float,max_fields> last{}, restore{}, written{};
    std::uint8_t pending{};
    bool valid{}, retiring{};
};
inline bool equal(float a,float b) noexcept { return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b); }
inline bool shape(const Curve& curve) noexcept {
    const auto maximum=std::numeric_limits<std::uintptr_t>::max();
    return curve.valid && curve.type && curve.curve && curve.curve<=maximum-0x28 &&
        curve.count>=2 && curve.count<=max_points && (curve.count_word&0x7fffffffu)==curve.count &&
        curve.points>=4 && curve.points<=maximum-(max_points*point_stride);
}
inline std::uintptr_t address(const Curve& curve,std::size_t field) noexcept {
    return field<curve.count ? curve.points+field*point_stride+point_y :
        curve.curve+(field==curve.count?0x20u:0x24u);
}
template<class Read> Identity identity(Read& read,const Curve& curve) noexcept {
    if(!shape(curve))return Identity::replaced;
    std::uintptr_t type{},points{};std::uint32_t count{};
    if(!read(curve.curve,type) || !read(curve.curve+points_at,points))return Identity::unavailable;
    if(type!=curve.type || points!=curve.points)return Identity::replaced;
    if(!read(points-4,count))return Identity::unavailable;
    if(count!=curve.count_word)return Identity::replaced;
    for(std::size_t i=0;i<curve.count;++i){
        float input{};if(!read(points+i*point_stride+point_x,input))return Identity::unavailable;
        if(!equal(input,curve.inputs[i]))return Identity::replaced;
    }
    return Identity::same;
}
template<class Read> Identity values(Read& read,const Curve& curve,std::array<float,max_fields>& out) noexcept {
    auto state=identity(read,curve);if(state!=Identity::same)return state;
    for(std::size_t i=0;i<curve.count+2u;++i){
        if(!read(address(curve,i),out[i]))return Identity::unavailable;
        if(!std::isfinite(out[i]))return Identity::replaced;
    }
    return identity(read,curve);
}
template<class Read> bool capture(Read& read,std::uintptr_t at,std::uintptr_t type,
        std::uint32_t count,float first_x,float first_y,Curve& out) noexcept {
    out={};Curve curve{};curve.curve=at;curve.type=type;curve.count=count;curve.valid=true;
    if(!at || at>std::numeric_limits<std::uintptr_t>::max()-0x28 || !type || count<2 || count>max_points ||
        !std::isfinite(first_x) || !std::isfinite(first_y) ||
        !read(at+points_at,curve.points) || curve.points<4 ||
        curve.points>std::numeric_limits<std::uintptr_t>::max()-max_points*point_stride ||
        !read(curve.points-4,curve.count_word) || (curve.count_word&0x7fffffffu)!=count)return false;
    for(std::size_t i=0;i<count;++i)
        if(!read(curve.points+i*point_stride+point_x,curve.inputs[i]) || !std::isfinite(curve.inputs[i]))return false;
    if(values(read,curve,curve.last)!=Identity::same || std::abs(curve.inputs[0]-first_x)>.01f ||
        std::abs(curve.last[0]-first_y)>.01f)return false;
    for(std::size_t i=0;i<count;++i)curve.outputs[i]=curve.last[i];
    curve.low=curve.last[count];curve.high=curve.last[count+1u];out=curve;return true;
}
inline bool needs_work(const Curve& curve,float factor) noexcept {
    return curve.retiring || curve.pending || !equal(curve.factor,factor);
}
// Only restore cells that still contain our exact write, under the same asset
// identity. Unreadable cells retain a bounded cleanup obligation for later ticks.
template<class Read,class CompareWrite> Result recover(Read& read,CompareWrite& write,Curve& curve) noexcept {
    const auto owner=identity(read,curve);
    if(owner==Identity::unavailable)return {Status::restore_pending};
    if(owner==Identity::replaced){curve.pending=0;curve.valid=false;return {Status::retired};}
    for(std::size_t i=0;i<curve.count+2u;++i){
        const auto bit=static_cast<std::uint8_t>(1u<<i);if(!(curve.pending&bit))continue;
        const auto current_owner=identity(read,curve);
        if(current_owner==Identity::unavailable)return {Status::restore_pending};
        if(current_owner==Identity::replaced){curve.pending=0;curve.valid=false;return {Status::retired};}
        float current{};if(!read(address(curve,i),current))continue;
        if(!equal(current,curve.written[i])){curve.pending=static_cast<std::uint8_t>(curve.pending&~bit);continue;}
        if(write(address(curve,i),curve.written[i],curve.restore[i]))curve.pending=static_cast<std::uint8_t>(curve.pending&~bit);
    }
    if(curve.pending)return {Status::restore_pending};
    curve.valid=false;return {Status::retired};
}
template<class Read,class CompareWrite> Result apply(Read& read,CompareWrite& write,Curve& curve,float factor) noexcept {
    if(curve.retiring)return recover(read,write,curve);
    if(!shape(curve) || !std::isfinite(factor) || !(factor>.01f && factor<100))return {Status::unavailable};
    std::array<float,max_fields> current{},target{};
    const auto owner=values(read,curve,current);
    if(owner==Identity::unavailable)return {Status::unavailable};
    if(owner==Identity::replaced){curve.valid=false;return {Status::retired};}
    for(std::size_t i=0;i<curve.count+2u;++i)
        if(!equal(current[i],curve.last[i])){curve.valid=false;return {Status::retired};}
    for(std::size_t i=0;i<curve.count;++i)target[i]=curve.outputs[i]*factor;
    target[curve.count]=curve.low>0?curve.low*factor:curve.low;
    target[curve.count+1u]=curve.high>0?curve.high*factor:curve.high;
    for(std::size_t i=0;i<curve.count+2u;++i)if(!std::isfinite(target[i]))return {Status::unavailable};
    curve.restore=current;curve.written=target;
    std::size_t changed_outputs{};bool changed_fields{};
    for(std::size_t i=0;i<curve.count+2u;++i){
        if(equal(current[i],target[i]))continue;
        std::array<float,max_fields> check{};
        bool owned=values(read,curve,check)==Identity::same;
        for(std::size_t j=0;j<curve.count+2u && owned;++j)owned=equal(check[j],current[j]);
        if(!owned || !write(address(curve,i),current[i],target[i])){
            curve.retiring=true;return recover(read,write,curve);
        }
        curve.pending=static_cast<std::uint8_t>(curve.pending|(1u<<i));current[i]=target[i];
        changed_fields=true;
        if(i<curve.count)++changed_outputs;
    }
    std::array<float,max_fields> after{};bool owned=values(read,curve,after)==Identity::same;
    for(std::size_t i=0;i<curve.count+2u && owned;++i)owned=equal(after[i],target[i]);
    if(!owned){curve.retiring=true;return recover(read,write,curve);}
    curve.pending=0;curve.last=target;curve.factor=factor;
    return {changed_fields?Status::applied:Status::unchanged,changed_outputs};
}
}
