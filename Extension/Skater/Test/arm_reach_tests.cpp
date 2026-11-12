// An arm reaching apart from the game: the hand joint lands on a point in reach, stops at the arm's
// length short of one out of reach, turns to a rotation asked for, and leaves a broken pose alone.
#include "Extension/Skater/arm_reach.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace dingosdk;
using namespace dingosdk::arm_reach;
namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
float distance(const Vec3& a, const Vec3& b) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}
bool same_turn(const Quat& a, const Quat& b) { return std::abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]) > 0.9999f; }
bool same_pose(const std::vector<Joint>& a, const std::vector<Joint>& b) {
    for (std::size_t i = 0; i < a.size(); ++i)
        if (!same_turn(a[i].rotation, b[i].rotation) || distance(a[i].position, b[i].position) > 1e-6f) return false;
    return true;
}

// A skater standing with both arms straight out: upper arms 0.3 m, forearms 0.28 m.
constexpr float upper_length = 0.3f, fore_length = 0.28f;
std::vector<Joint> t_pose() {
    std::vector<Joint> pose(395);
    pose[0].position = {10, 0, 5};
    pose[1].position = {0, 1.0f, 0};
    pose[45].position = {0, 0.4f, 0};
    pose[0].rotation = anim_studio::from_euler({0, 30, 0}); // the skater turned, to keep the maths honest
    for (const auto hand : {Hand::right, Hand::left}) {
        const auto joints = chain(hand);
        const float x = hand == Hand::left ? -1.0f : 1.0f;
        pose[joints[7]].position = {0.15f * x, 0, 0};
        pose[joints[8]].position = {0.1f * x, 0, 0};
        pose[joints[9]].position = {upper_length * x, 0, 0};
        pose[joints[10]].position = {fore_length * x, 0, 0};
    }
    return pose;
}
Vec3 joint_at(const std::vector<Joint>& pose, Hand hand, std::size_t index) {
    std::array<Placed, 11> w{};
    check(compose(pose, chain(hand), w), "The arm composes");
    return w[index].position;
}

void a_point_in_reach_is_reached() {
    for (const auto side : {Hand::right, Hand::left}) {
        auto pose = t_pose();
        const Vec3 s = joint_at(pose, side, 8);
        Goal goal;
        goal.hand = side;
        goal.at = {s[0] + 0.1f, s[1] - 0.25f, s[2] + 0.3f};
        const auto r = reach(pose, goal);
        check(r.valid && r.reached, "A point in reach is reached");
        check(distance(r.hand.position, goal.at) < 2e-3f, "and the hand is on it");
        check(distance(hand_of(pose, side).hand.position, goal.at) < 2e-3f, "as the pose now has it");
        check(std::abs(distance(joint_at(pose, side, 8), joint_at(pose, side, 9)) - upper_length) < 1e-4f, "The upper arm keeps its length");
        check(std::abs(distance(joint_at(pose, side, 9), joint_at(pose, side, 10)) - fore_length) < 1e-4f, "and the forearm its length");
    }
}

void a_point_out_of_reach_is_pointed_at() {
    auto pose = t_pose();
    const Vec3 s = joint_at(pose, Hand::right, 8);
    Goal goal;
    goal.at = {s[0], s[1] + 3.0f, s[2]};
    const auto r = reach(pose, goal);
    check(r.valid && !r.reached, "A point out of reach is not reached");
    check(std::abs(distance(r.hand.position, s) - (upper_length + fore_length) * 0.995f) < 2e-3f, "The arm stops just short of straight");
    check(r.hand.position[1] - s[1] > 0.5f, "pointing at it");
}

void a_rotation_asked_for_is_turned_to() {
    auto pose = t_pose();
    const Vec3 s = joint_at(pose, Hand::right, 8);
    Goal goal;
    goal.at = {s[0] + 0.2f, s[1] - 0.2f, s[2] + 0.2f};
    goal.rotation = anim_studio::from_euler({20, -70, 45});
    const auto r = reach(pose, goal);
    check(r.valid && same_turn(r.hand.rotation, *goal.rotation), "The hand turns to the rotation asked for");
    check(distance(r.hand.position, goal.at) < 2e-3f, "without leaving the point");
}

void no_weight_is_the_animation() {
    auto pose = t_pose();
    const auto before = pose;
    Goal goal;
    goal.at = joint_at(pose, Hand::right, 8);
    goal.at[2] += 0.4f;
    goal.weight = 0;
    check(reach(pose, goal).valid && same_pose(pose, before), "A weight of 0 leaves the arm as it was");
}

void only_the_arm_is_written() {
    auto pose = t_pose();
    const auto before = pose;
    Goal goal;
    goal.hand = Hand::left;
    goal.at = joint_at(pose, Hand::left, 8);
    goal.at[1] -= 0.4f;
    goal.rotation = anim_studio::from_euler({0, 0, 90});
    check(reach(pose, goal).valid, "The left arm reaches");
    for (std::size_t i = 0; i < pose.size(); ++i)
        if (i < first_written(Hand::left) || i > last_written(Hand::left))
            check(same_turn(pose[i].rotation, before[i].rotation), "A reach writes only the joints it says it writes");
}

void a_broken_pose_is_left_alone() {
    auto pose = t_pose();
    pose[48].rotation[0] = NAN;
    const auto before = pose;
    Goal goal;
    goal.at = {10, 1, 5};
    check(!reach(pose, goal).valid && !hand_of(pose, Hand::right).valid, "A broken arm is not reached with");
    for (std::size_t i = 0; i < pose.size(); ++i)
        if (i != 48) check(same_turn(pose[i].rotation, before[i].rotation), "and nothing is written");
    auto short_pose = t_pose();
    short_pose.resize(40);
    check(!reach(short_pose, goal).valid, "A pose without the arm's joints is refused");
    auto stretched = t_pose();
    stretched[48].position = {3, 0, 0};
    check(!reach(stretched, goal).valid, "An arm not an arm's length is refused");
    goal.at = {NAN, 0, 0};
    auto fine = t_pose();
    check(!reach(fine, goal).valid, "A point that is not a number is refused");
}
void held_endpoint_uses_native_placement_without_stretching() {
    for(const auto tip:{std::uint16_t{10},std::uint16_t{343}}) {
        auto pose=t_pose();
        const auto middle=static_cast<std::uint16_t>(anim_studio::parent(tip));
        const auto upper_joint=static_cast<std::uint16_t>(anim_studio::parent(middle));
        pose[0].rotation=anim_studio::from_euler({13,42,-9});
        pose[upper_joint].position={.15f,0,0};
        pose[middle].position={0,-.45f,.1f};pose[tip].position={0,-.4f,-.1f};
        const auto before=pose;
        const auto root=anim_studio::world(pose,upper_joint);
        const auto bend=anim_studio::world(pose,middle),foot=anim_studio::world(pose,tip);
        const Vec3 target{root.position[0]+.3f,root.position[1]-.6f,root.position[2]+.15f};
        const auto reached=reach_joint(pose,tip,target,1);
        check(reached.valid && reached.reached && distance(reached.hand.position,target)<2e-4f,
              "Both held feet reach a real world point from a pitched and turned native placement");
        check(same_turn(reached.hand.rotation,foot.rotation),"The authored foot orientation is retained");
        check(std::abs(distance(anim_studio::world(pose,upper_joint).position,anim_studio::world(pose,middle).position)-
            distance(root.position,bend.position))<1e-4f,"A held contact does not stretch the upper leg");
        check(std::abs(distance(anim_studio::world(pose,middle).position,anim_studio::world(pose,tip).position)-
            distance(bend.position,foot.position))<1e-4f,"A held contact does not stretch the lower leg");
        for(std::size_t i=0;i<pose.size();++i) {
            check(pose[i].position==before[i].position && pose[i].scale==before[i].scale,
                  "Contact placement never translates or scales any joint");
            if(i!=upper_joint && i!=middle && i!=tip)
                check(same_turn(pose[i].rotation,before[i].rotation),"Contact leaves the root, board and unrelated limbs alone");
        }
        pose=before;
        check(reach_joint(pose,tip,target,0).valid && same_pose(pose,before),"Zero contact blend is exactly the authored pose");
        const Vec3 far{root.position[0]+3,root.position[1],root.position[2]};
        const auto refused=reach_joint(pose,tip,far,1);
        check(refused.valid && !refused.reached && same_pose(pose,before),"Unreachable contact is reported without moving or stretching the skater");
        check(!reach_joint(pose,tip,{NAN,0,0},1).valid && same_pose(pose,before),"Invalid contact leaves the pose alone");
        check(!reach_joint(pose,1,target,1).valid && same_pose(pose,before),"World placement cannot be used as a limb");
        check(!reach_joint(pose,tip,target,NAN).valid && same_pose(pose,before),"Invalid contact blend leaves the pose alone");
    }
}
void contact_plane_preserves_the_foot_height() {
    // A torso collision point is not a reachable foot endpoint. Preserve the
    // drawn foot's height and tangential position on the same observed plane.
    const Vec3 foot{128.25677f,1.2839673f,-136.03656f};
    const Vec3 point{128.21747f,1.5892204f,-136.2787f};
    const auto target=plane_point(foot,point,{0,0,1});
    check(target && (*target)[0]==foot[0] && (*target)[1]==foot[1] && (*target)[2]==point[2],
          "Observed body contact defines a plane without dragging the foot to torso height");
    check(plane_point(foot,point,{0,0,-2})==target,"Plane projection is independent of normal scale and sign");
    const auto angled=plane_point({3,4,5},{1,2,3},{1,0,1});
    check(angled && distance(*angled,{1,4,3})<1e-5f,"Angled wall projection removes only normal displacement");
    check(target && plane_point(*target,point,{0,0,1})==target,"A point already on the wall remains fixed");
    check(!plane_point(foot,point,{0,0,0}) && !plane_point(foot,point,{NAN,0,1}) &&
          !plane_point({INFINITY,0,0},point,{0,0,1}) && !plane_point(foot,{0,NAN,0},{0,0,1}),
          "Invalid surface or pose cannot produce a contact target");
}
void authored_contact_frame_transfers_without_root_motion() {
    for(const auto tip:{std::uint16_t{10},std::uint16_t{343}}) {
        Joint source,support;
        support.rotation=anim_studio::from_euler({25,-40,19});
        support.position={3,4,-2};
        const auto up=anim_studio::rotate(support.rotation,{0,1,0});
        source.rotation=anim_studio::from_euler({-23,67,11});
        source.scale={2,2,2};
        for(std::size_t k=0;k<3;++k)source.position[k]=support.position[k]+up[k]*.24f;
        const auto frame=contact_frame(source,support);
        check(frame && std::abs(frame->clearance-.12f)<1e-5f,"Authored clearance removes source scale exactly once");
        check(distance(anim_studio::rotate(source.rotation,frame->normal),up)<1e-5f,
            "Contact normal comes from the rotated authored plane, not a guessed foot axis");
        auto pose=t_pose();
        const auto middle=static_cast<std::uint16_t>(anim_studio::parent(tip));
        const auto upper_joint=static_cast<std::uint16_t>(anim_studio::parent(middle));
        pose[middle].position={0,-.45f,.1f};pose[tip].position={0,-.4f,-.1f};
        const auto before=pose;
        const auto foot=anim_studio::world(pose,tip);
        const auto root=anim_studio::world(pose,upper_joint);
        const Vec3 plane{foot.position[0],root.position[1]-.6f,foot.position[2]-.12f};
        const auto target=contact_target(foot,*frame,plane,{0,0,2});
        check(target && std::abs(target->position[2]-plane[2]-.12f)<1e-5f,
            "Joint origin stays outside the observed plane by the authored clearance");
        auto at=target->position;at[1]=plane[1];
        const auto reached=reach_joint(pose,tip,at,1,target->rotation);
        check(reached.valid && reached.reached && distance(reached.hand.position,at)<2e-4f,
            "Both legs reach the supported joint origin");
        check(distance(anim_studio::rotate(reached.hand.rotation,frame->normal),{0,0,1})<2e-4f,
            "The actual reached endpoint contact normal aligns with the wall");
        for(std::size_t i=0;i<pose.size();++i) {
            check(pose[i].position==before[i].position && pose[i].scale==before[i].scale,
                "Contact orientation changes no joint translations or scales");
            if(i!=tip && i!=middle && i!=upper_joint)
                check(same_turn(pose[i].rotation,before[i].rotation),"Root, board and unrelated joints remain native");
        }
        pose=before;
        check(!reach_joint(pose,tip,at,1,Quat{0,0,0,0}).valid && same_pose(pose,before),
            "Invalid orientation cannot write a limb");
        const auto far=reach_joint(pose,tip,{100,100,100},1,target->rotation);
        check(far.valid && !far.reached && same_pose(pose,before),"Unreachable oriented contact does not stretch or teleport");
        auto larger=foot;larger.scale={1.5f,1.5f,1.5f};
        const auto scaled=contact_target(larger,*frame,plane,{0,0,1});
        check(scaled && std::abs(scaled->position[2]-plane[2]-.18f)<1e-5f,"Current native scale applies clearance once");
        larger.scale[1]=2;
        check(!contact_target(larger,*frame,plane,{0,0,1}),"Nonuniform scale cannot silently change the contact frame");
        auto invalid=*frame;invalid.clearance=-.01f;
        check(!contact_target(foot,invalid,plane,{0,0,1}),"Negative clearance cannot embed the endpoint");
        invalid=*frame;invalid.normal={0,0,0};
        check(!contact_target(foot,invalid,plane,{0,0,1}),"Missing source normal is refused");
        check(!contact_target(foot,*frame,plane,{0,0,0}),"Missing collision plane is refused");
    }
}
} // namespace

int main() {
    try {
        a_point_in_reach_is_reached();
        a_point_out_of_reach_is_pointed_at();
        a_rotation_asked_for_is_turned_to();
        no_weight_is_the_animation();
        only_the_arm_is_written();
        a_broken_pose_is_left_alone();
        held_endpoint_uses_native_placement_without_stretching();
        contact_plane_preserves_the_foot_height();
        authored_contact_frame_transfers_without_root_motion();
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "arm_reach: all passed\n";
    return 0;
}
