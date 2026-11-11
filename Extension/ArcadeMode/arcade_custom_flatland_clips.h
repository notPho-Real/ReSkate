#pragma once
#include "arcade_custom_flatland.h"
#include "Extension/AnimationStudio/motion_authoring.h"
#include "Extension/AnimationStudio/board_geometry.h"
#include <stdexcept>
#include "arcade_casper_motion.inl"

namespace dingosdk::arcade_mode {
// Newly authored motion in the board frame. A live Manual pose supplies the
// skeleton proportions and stance; no source-game animation is embedded.
inline CustomTrickClips make_casper_clips(std::span<const anim_studio::Joint> seed,
        const anim_studio::Joint& board) {
    using namespace anim_studio;
    namespace author=motion_authoring;
    using namespace pose_math;
    if(seed.size()!=skeleton_joints || !anim_studio::finite(board))
        throw std::invalid_argument("flatland seed");
    for(const auto& joint:seed)if(!anim_studio::finite(joint))throw std::invalid_argument("flatland joint");
    auto base=std::vector<Joint>(seed.begin(),seed.end());
    // Author in the placement frame. Restore these two untouched joints after
    // solving; the player's native placement remains authoritative.
    base[0]=base[placement_joint]=Joint{};
    const auto left=world(base,343),right=world(base,10);
    const bool left_front=author::unpoint(board,left.position)[2]>author::unpoint(board,right.position)[2];
    const std::array<std::uint16_t,3> front=left_front?std::array<std::uint16_t,3>{341,342,343}:std::array<std::uint16_t,3>{8,9,10};
    const std::array<std::uint16_t,3> back=left_front?std::array<std::uint16_t,3>{8,9,10}:std::array<std::uint16_t,3>{341,342,343};
    const auto front_foot=world(base,front[2]),back_foot=world(base,back[2]);
    const auto shape=board_geometry::from_pose(base,board);
    // Ground is the native skeleton's support height in the placement frame.
    // Only the displayed board is fitted to it; rigid bodies remain native.
    constexpr std::uint16_t ground=375;
    const float support=world(base,ground).position[1];
    const float side=left_front?1.f:-1.f;
    // Keep heading, but remove the incoming Manual's pitch/roll as contact is
    // established. Authored hold pitch is measured from the support plane.
    const auto forward=rotate(board.rotation,{0,0,1});
    const auto level=axis_angle({0,1,0},std::atan2(forward[0],forward[2]));
    const auto axis=[&](Vec3 v){return rotate(level,v);};
    // All numbers below are authored controls, not a second runtime pose layer.
    // Feet: the front instep supports the grip side; the rear sole rests on the
    // inverted tail. Release keys lift both feet before the board turns past them.
    using casper_motion::Key;
    const auto pose=[&](const Key& k,float balance=0.f) {
        author::PoseKey out;out.time=k.time;out.pose=base;out.board=board;out.weight=k.weight;
        const auto basis=slerp(board.rotation,level,k.upper);
        out.board.rotation=normalised(multiply(basis,from_euler({k.pitch,0,side*k.roll})));
        out.board.position=add(board.position,axis({0,k.lift+(author::unpoint(board,back_foot.position)[1]-.08f)*k.contact,0}));
        out.board.position[1]+=(support-board_geometry::lowest_point(out.board,shape))*k.contact;
        out.pose[hips_joint].position=add(out.pose[hips_joint].position,axis({0,-k.hip_drop,k.hip_back+.05f*balance}));
        author::turn(out.pose,hips_joint,axis_angle(axis({1,0,0}),(-4*k.upper+6*balance)*.01745329252f));
        author::turn(out.pose,42,axis_angle(axis({1,0,0}),(7*k.upper-12*balance)*.01745329252f));
        author::turn(out.pose,44,axis_angle(axis({0,0,1}),side*(5*k.upper+10*balance)*.01745329252f));
        author::turn(out.pose,103,axis_angle(axis({1,0,0}),-5*k.upper*.01745329252f));
        const auto foot_target=[&](const Joint& native,Vec3 contact,float lift,float lateral) {
            const auto on_board=author::point(out.board,contact);
            const auto target=add(native.position,scaled(sub(on_board,native.position),k.contact));
            const float outward=author::unpoint(board,native.position)[0]<0?-1.f:1.f;
            return add(target,axis({outward*lateral,lift,0}));
        };
        const auto front_target=foot_target(front_foot,{side*.025f,.025f,.25f},k.front_lift,k.front_side);
        const auto back_target=foot_target(back_foot,{-side*.015f,-.10f,-.32f},k.back_lift,k.back_side);
        const std::array<author::ReachTarget,2> supports{{{front,front_target},{back,back_target}}};
        (void)author::lower_for_reach(out.pose,hips_joint,supports,.15f*k.contact);
        const auto foot=[&](const std::array<std::uint16_t,3>& chain,const Joint& native,
                const Vec3& target,float ankle) {
            const auto level_foot=normalised(multiply(multiply(level,conjugate(board.rotation)),native.rotation));
            const auto rotation=normalised(multiply(axis_angle(axis({1,0,0}),ankle*k.contact*.01745329252f),
                slerp(native.rotation,level_foot,k.contact)));
            // Preserve the native knee's side of the leg; use its bend plane,
            // with the skater's forward direction only as a straight-leg fallback.
            const auto hip=world(base,chain[0]),knee=world(base,chain[1]);
            const auto middle=scaled(add(hip.position,native.position),.5f);
            const auto pole=unit(sub(knee.position,middle),axis({side,0,0}));
            (void)author::reach(out.pose,chain,target,pole,rotation);
        };
        foot(front,front_foot,front_target,-6);
        foot(back,back_foot,back_target,5);
        // Open hands counter the body transfer; each arm keeps its own lengths.
        for(const auto chain: {std::array<std::uint16_t,3>{47,48,49},std::array<std::uint16_t,3>{276,277,278}}) {
            const auto native=world(base,chain[2]),shoulder=world(out.pose,chain[0]);
            auto across=sub(shoulder.position,world(out.pose,45).position);
            across[1]=0; // A raised native clavicle must not lift the hand target.
            const auto outward=unit(across,axis({1,0,0}));
            const float response=(chain[0]==47?1.f:-1.f)*balance;
            const auto balanced=add(shoulder.position,add(scaled(outward,.35f+.05f*std::abs(balance)),
                add(Vec3{0,-.18f+.075f*response,0},axis({0,0,-.04f+.10f*response}))));
            const auto target=add(native.position,scaled(sub(balanced,native.position),k.upper));
            (void)author::reach(out.pose,chain,target,add(Vec3{0,-1,0},axis({0,0,-.3f})),native.rotation);
            // Let the wrist follow the solved forearm instead of holding its
            // old world orientation and introducing a new wrist bend.
            out.pose[chain[2]].rotation=base[chain[2]].rotation;
        }
        if(k.contact==0 && k.front_lift==0 && k.back_lift==0 && k.front_side==0 && k.back_side==0 && k.upper==0 &&
            k.hip_drop==0 && k.hip_back==0)out.pose.assign(seed.begin(),seed.end());
        out.pose[0]=seed[0];out.pose[placement_joint]=seed[placement_joint];
        return out;
    };
    std::vector<float> mask(skeleton_joints,0);
    for(const auto& joint:editable_joints())if(joint.index<mask.size())mask[joint.index]=1;
    const auto phase=[&](std::string name){return author::Phase{std::move(name),{},mask};};
    auto entry=phase("Casper Entry");
    for(const auto& k:casper_motion::entry)entry.keys.push_back(pose(k));
    auto init=author::bake(entry);
    auto out=author::reversed(init);
    for(std::size_t f=0;f<out.times.size();++f)out.weights[f]=1-author::smooth((out.times[f]-.20f)/.10f);
    auto hold=phase("Casper Range");hold.ease=false;
    Key held=casper_motion::hold.front();
    const float held_pitch=held.pitch;
    for(float balance:{-1.f,-.5f,0.f,.5f,1.f}) {
        held.time=2+balance;held.pitch=held_pitch+2*balance;
        hold.keys.push_back(pose(held,balance));
    }
    const auto range=author::bake(hold);
    // Preserve the original entry/hold handle and its finite fallback lease.
    // Externally clocked playback addresses only the entry or the range.
    for(std::size_t f=0;f<range.times.size();++f) {
        init.times.push_back(range.times[f]);
        init.frames.insert(init.frames.end(),range.frame(f).begin(),range.frame(f).end());
        init.board.push_back(range.board[f]);init.weights.push_back(1);
    }
    init.times.push_back(15);init.frames.insert(init.frames.end(),seed.begin(),seed.end());
    init.board.push_back(board);init.weights.push_back(0);
    auto flip=phase("Casper Flip");
    for(const auto& k:casper_motion::flip)flip.keys.push_back(pose(k));
    return {std::make_shared<Clip>(std::move(init)),std::make_shared<Clip>(author::bake(flip)),
        std::make_shared<Clip>(std::move(out)),1,3,{.08f,.16f}};
}
}
