#pragma once
#include "anim_core.h"
#include "pose_math.h"
#include <stdexcept>

// Authoring helpers that bake into the existing Clip format. They do not own a
// player, clock, balance model or physical contacts.
namespace dingosdk::anim_studio::motion_authoring {
using namespace pose_math;
inline float smooth(float t) noexcept {t=std::clamp(t,0.f,1.f);return t*t*(3-2*t);}
// Offset an authored balance range for quiet weight shifts and quicker
// corrections near either limit. This changes presentation only. The caller
// advances seconds from its admitted native steps and retains balance ownership.
struct RangeMotion {
    float idle_amplitude{},edge_amplitude{};
    static constexpr float period=4.f;
    float sample(float balance,float seconds) const noexcept {
        if(!std::isfinite(balance) || !std::isfinite(seconds))return 0;
        const float b=std::clamp(balance,-1.f,1.f);
        const float edge=smooth((std::abs(b)-.55f)/.45f);
        const float phase=std::remainder(seconds,period)*6.28318530718f;
        const float sway=idle_amplitude*(1-edge)*std::sin(phase*.75f);
        const float shake=edge_amplitude*edge*std::sin(phase*4.5f);
        return std::clamp(b+sway+shake,-1.f,1.f);
    }
};
inline Joint mix(const Joint& a,const Joint& b,float t) noexcept {
    Joint out;out.rotation=slerp(a.rotation,b.rotation,t);
    for(unsigned i=0;i<3;++i){out.position[i]=a.position[i]+(b.position[i]-a.position[i])*t;
        out.scale[i]=a.scale[i]+(b.scale[i]-a.scale[i])*t;}
    return out;
}
inline Vec3 point(const Joint& frame,const Vec3& p) noexcept {
    Vec3 v=p;for(unsigned i=0;i<3;++i)v[i]*=frame.scale[i];
    return add(frame.position,rotate(frame.rotation,v));
}
inline Vec3 unpoint(const Joint& frame,const Vec3& p) noexcept {
    auto v=rotate(conjugate(frame.rotation),sub(p,frame.position));
    for(unsigned i=0;i<3;++i)v[i]/=frame.scale[i];
    return v;
}
// Place a particular point of a rigid joint frame on a support target. An
// ankle pivot is not the sole or instep; author contacts using their own anchor.
inline Joint anchored(Joint frame,const Vec3& local_anchor,const Vec3& target) {
    if(!anim_studio::finite(frame) || !pose_math::finite(local_anchor) || !pose_math::finite(target))
        throw std::invalid_argument("invalid authored contact anchor");
    frame.position=add(frame.position,sub(target,point(frame,local_anchor)));
    return frame;
}
// Turn a joint in the composed frame without changing any bone length.
inline void turn(std::span<Joint> pose,std::uint16_t joint,const Quat& delta) noexcept {
    const auto p=parent(joint);
    const auto parent_rotation=p>=0?world(pose,static_cast<std::uint16_t>(p)).rotation:Quat{0,0,0,1};
    pose[joint].rotation=normalised(multiply(conjugate(parent_rotation),
        multiply(delta,multiply(parent_rotation,pose[joint].rotation))));
}
// The same analytic two-bone construction used by the existing arm reach,
// generalized to an authored chain and pole. Returns actual endpoint error;
// unreachable targets clamp, never stretch or translate limb bones.
inline float reach(std::span<Joint> pose,std::array<std::uint16_t,3> chain,
        const Vec3& target,const Vec3& pole,const Quat& rotation) {
    if(chain[2]>=pose.size() || parent(chain[2])!=chain[1] || parent(chain[1])!=chain[0] ||
        !pose_math::finite(target) || !pose_math::finite(pole) || !pose_math::finite(rotation))
        throw std::invalid_argument("invalid authored limb");
    const auto a=world(pose,chain[0]),b=world(pose,chain[1]),c=world(pose,chain[2]);
    const float upper=length(sub(b.position,a.position)),lower=length(sub(c.position,b.position));
    if(!(upper>.05f && lower>.05f && upper<1.2f && lower<1.2f))
        throw std::invalid_argument("invalid authored limb proportions");
    const auto direction=unit(sub(target,a.position),{0,-1,0});
    const float d=std::clamp(length(sub(target,a.position)),std::abs(upper-lower)+.001f,(upper+lower)*.999f);
    const auto plane=unit(sub(pole,scaled(direction,dot(pole,direction))),{0,0,1});
    const float cosine=std::clamp((upper*upper+d*d-lower*lower)/(2*upper*d),-1.f,1.f);
    const auto bend=add(a.position,scaled(add(scaled(direction,cosine),
        scaled(plane,std::sqrt(std::max(0.f,1-cosine*cosine)))),upper));
    turn(pose,chain[0],arc(sub(b.position,a.position),sub(bend,a.position)));
    const auto moved_b=world(pose,chain[1]),moved_c=world(pose,chain[2]);
    const auto end=add(a.position,scaled(direction,d));
    turn(pose,chain[1],arc(sub(moved_c.position,moved_b.position),sub(end,moved_b.position)));
    pose[chain[2]].rotation=normalised(multiply(conjugate(world(pose,chain[1]).rotation),rotation));
    return length(sub(world(pose,chain[2]).position,target));
}
struct ReachTarget {
    std::array<std::uint16_t,3> chain;
    Vec3 target;
};
// Lower an authored root only as much as its target legs require. Without this,
// a low board target can lie beyond a tall native Manual's leg reach and the
// analytic solver correctly clamps, leaving the displayed shoe above the deck.
inline float lower_for_reach(std::span<Joint> pose,std::uint16_t root,
        std::span<const ReachTarget> targets,float limit) {
    if(root>=pose.size() || !std::isfinite(limit) || limit<0 || limit>.5f)
        throw std::invalid_argument("invalid authored support fit");
    float drop=0;
    for(const auto& item:targets) {
        if(item.chain[2]>=pose.size() || item.chain[1]>=pose.size() || item.chain[0]>=pose.size() ||
            !pose_math::finite(item.target))throw std::invalid_argument("invalid authored support target");
        const auto a=world(pose,item.chain[0]),b=world(pose,item.chain[1]),c=world(pose,item.chain[2]);
        const float reach=(length(sub(b.position,a.position))+length(sub(c.position,b.position)))*.985f;
        const auto delta=sub(a.position,item.target);
        const float vertical=std::sqrt(std::max(0.f,reach*reach-delta[0]*delta[0]-delta[2]*delta[2]));
        drop=std::max(drop,delta[1]-vertical);
    }
    drop=std::min(drop,limit);
    if(drop>0) {
        const auto p=parent(root);
        const auto frame=p>=0?world(pose,static_cast<std::uint16_t>(p)):Joint{};
        pose[root].position=unpoint(frame,sub(world(pose,root).position,{0,drop,0}));
    }
    return drop;
}
struct PoseKey {
    float time{};
    std::vector<Joint> pose;
    Joint board;
    float weight{1};
};
struct Phase {
    std::string name;
    std::vector<PoseKey> keys;
    std::vector<float> joint_weights;
    bool ease{true}; // Parameter ranges need linear response through their center.
};
// Bake eased keys densely enough that long board rotations retain their path.
// Supply intermediate keys for rotations exceeding 180 degrees.
inline Clip bake(const Phase& phase,unsigned hz=120) {
    if(phase.name.empty() || phase.keys.size()<2 || hz<1 || hz>240)
        throw std::invalid_argument("invalid authored phase");
    const auto count=phase.keys.front().pose.size();
    if(count==0 || count>512 || phase.keys.size()>20000)throw std::invalid_argument("invalid authored dimensions");
    Clip out;out.joints=static_cast<std::uint32_t>(count);out.joint_weights=phase.joint_weights;
    for(std::size_t k=0;k<phase.keys.size();++k) {
        const auto& key=phase.keys[k];
        if(key.pose.size()!=count || !std::isfinite(key.time) || key.time<0 || key.time>20000.f/static_cast<float>(hz) ||
            !std::isfinite(key.weight) || key.weight<0 || key.weight>1 ||
            (k && key.time<=phase.keys[k-1].time))throw std::invalid_argument("invalid authored key");
        for(const auto& joint:key.pose)if(!anim_studio::finite(joint))throw std::invalid_argument("invalid authored pose");
        if(!anim_studio::finite(key.board))throw std::invalid_argument("invalid authored board");
        if(!k)continue;
        const auto& previous=phase.keys[k-1];
        const auto steps=static_cast<unsigned>(std::ceil((key.time-previous.time)*static_cast<float>(hz)));
        if(out.times.size()+steps+1>20000)throw std::invalid_argument("authored clip too large");
        for(unsigned j=0;j<steps;++j) {
            const float u=static_cast<float>(j)/static_cast<float>(steps),w=phase.ease?smooth(u):u;
            out.times.push_back(previous.time+(key.time-previous.time)*u);
            for(std::size_t bone=0;bone<count;++bone)out.frames.push_back(mix(previous.pose[bone],key.pose[bone],w));
            out.board.push_back(mix(previous.board,key.board,w));
            out.weights.push_back(previous.weight+(key.weight-previous.weight)*w);
        }
    }
    const auto& last=phase.keys.back();out.times.push_back(last.time);
    out.frames.insert(out.frames.end(),last.pose.begin(),last.pose.end());out.board.push_back(last.board);out.weights.push_back(last.weight);
    // The same bounded validator is used for imported and authored clips.
    return clip_from_bytes(clip_bytes(out));
}
inline Clip reversed(const Clip& source) {
    Clip out=source;const auto count=source.times.size();
    for(std::size_t i=0;i<count;++i) {
        out.times[i]=source.duration()-source.times[count-1-i];
        std::copy(source.frame(count-1-i).begin(),source.frame(count-1-i).end(),out.frames.begin()+static_cast<std::ptrdiff_t>(i*source.joints));
        if(!source.board.empty())out.board[i]=source.board[count-1-i];
        if(!source.weights.empty())out.weights[i]=source.weights[count-1-i];
    }
    out.takeoff=source.landing<0?-1:source.duration()-source.landing;
    out.landing=source.takeoff<0?-1:source.duration()-source.takeoff;
    return out;
}
// Join a range-selected pose to an exit without resetting the player's blend.
// Only a short lead-in is adjusted; the rest of the authored exit is retained.
inline Clip joined(const Clip& destination,const Clip& source,float source_time,float seconds=.10f) {
    Clip out=destination;std::vector<Joint> from(source.joints);source.sample(source_time,from);
    Joint board;const bool has_board=source.sample_board(source_time,board);
    if(source.joints!=out.joints || !(seconds>0))throw std::invalid_argument("invalid phase join");
    for(std::size_t f=0;f<out.times.size() && out.times[f]<seconds;++f) {
        const float w=smooth(out.times[f]/seconds);
        for(std::size_t j=0;j<out.joints;++j)out.frames[f*out.joints+j]=mix(from[j],out.frames[f*out.joints+j],w);
        if(has_board && !out.board.empty())out.board[f]=mix(board,out.board[f],w);
    }
    return out;
}
} // namespace dingosdk::anim_studio::motion_authoring
