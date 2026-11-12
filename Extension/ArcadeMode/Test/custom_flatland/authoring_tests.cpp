#include "pose_fixture.h"
#include "Extension/ArcadeMode/arcade_custom_flatland_clips.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::anim_studio;
using namespace dingosdk::anim_studio::motion_authoring;
using dingosdk::arcade_mode::make_casper_clips;
namespace {
unsigned checks{};
void check(bool ok,const char* why){++checks;if(!ok){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
template<class F>void rejected(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"invalid authored/imported data rejected");}
bool same(const Joint& a,const Joint& b,float epsilon=.0001f){
    float dot=0;for(unsigned i=0;i<4;++i)dot+=a.rotation[i]*b.rotation[i];
    return std::abs(std::abs(dot)-1)<epsilon && length(sub(a.position,b.position))<epsilon && a.scale==b.scale;
}
void clips(){
    for(bool goofy:{false,true})for(float size:{.9f,1.f,1.1f}) {
        auto seed=flatland_pose_fixture(goofy);
        for(auto& j:seed)j.position=scaled(j.position,size);
        seed[0].rotation=from_euler({0,27,0});seed[0].position={4,1,2};
        seed[1].rotation=from_euler({0,58,0});seed[1].position={3,0,5};
        const auto c=make_casper_clips(seed,{});
        std::vector<Joint> pose(skeleton_joints);Joint board;
        c.entry_hold->sample(0,pose);
        for(std::size_t j=0;j<pose.size();++j) {
            if(!same(seed[j],pose[j]))std::cerr<<"seed mismatch joint "<<j<<" size "<<size<<" goofy "<<goofy
                <<" position error "<<length(sub(seed[j].position,pose[j].position))
                <<" rotations "<<seed[j].rotation[0]<<","<<seed[j].rotation[1]<<","<<seed[j].rotation[2]<<","<<seed[j].rotation[3]
                <<" vs "<<pose[j].rotation[0]<<","<<pose[j].rotation[1]<<","<<pose[j].rotation[2]<<","<<pose[j].rotation[3]
                <<" scales "<<seed[j].scale[0]<<","<<seed[j].scale[1]<<","<<seed[j].scale[2]
                <<" vs "<<pose[j].scale[0]<<","<<pose[j].scale[1]<<","<<pose[j].scale[2]<<"\n";
            check(same(seed[j],pose[j]),"entry begins at the native seed");
        }
        c.entry_hold->sample(.3f,pose);std::vector<Joint> center(skeleton_joints);
        c.entry_hold->sample(2,center);
        for(std::size_t j=0;j<pose.size();++j)check(same(pose[j],center[j]),"entry joins the middle balance pose");
        const float side=goofy?-1.f:1.f;
        const auto front=static_cast<std::uint16_t>(goofy?10:343),back=static_cast<std::uint16_t>(goofy?343:10);
        for(unsigned f=0;f<=100;++f) {
            const float time=1+static_cast<float>(f)/50;
            c.entry_hold->sample(time,pose);c.entry_hold->sample_board(time,board);
            check(same(seed[0],pose[0]) && same(seed[1],pose[1]),"native placement is never authored");
            pose[0]=pose[1]=Joint{};
            const auto a=world(pose,front),b=world(pose,back);
            const auto toe=unpoint(board,world(pose,static_cast<std::uint16_t>(front+1)).position);
            check(std::abs(unpoint(board,a.position)[0])>.15f,"front ankle stays outside the board across balance");
            check(toe[1]>.025f && toe[1]<.065f && std::abs(toe[0])>.115f && std::abs(toe[0])<.145f,
                "front forefoot supports the grip side near the rail");
            check(length(sub(b.position,point(board,{-side*.015f,-.10f,-.32f})))<.015f,"rear sole stays at its authored inverted-tail target across balance");
            check(rotate(board.rotation,{0,1,0})[1]<-.5f,"range keeps trucks above the deck");
            auto unplaced=seed;unplaced[0]=unplaced[1]=Joint{};
            const auto shape=board_geometry::from_pose(unplaced,Joint{});
            check(std::abs(board_geometry::lowest_point(board,shape)-world(unplaced,375).position[1])<.002f,
                "held cosmetic board support follows native ground across balance and proportions");
            for(const auto arm:{std::array<std::uint16_t,2>{47,49},std::array<std::uint16_t,2>{276,278}})
                check(world(pose,arm[1]).position[1]<world(pose,arm[0]).position[1],
                    "held hands balance below their shoulders");
            for(auto j:{8u,9u,10u,341u,342u,343u})check(pose[j].position==seed[j].position,"contact solve never stretches limb bones");
        }
        for(const auto& clip:{c.entry_hold,c.flip_exit,c.direct_exit}) {
            const auto round=clip_from_bytes(clip_bytes(*clip));
            check(round.weights==clip->weights && round.joint_weights==clip->joint_weights,"blend envelope and joint mask survive export/import");
            check(round.joint_weight(107)==0 && round.joint_weight(343)==1,"face stays native while the foot is authored");
            for(const auto& joint:round.frames)check(dingosdk::anim_studio::finite(joint),"all baked body keys are finite");
        }
        for(float time:{.23f,.31f,.39f}) {
            c.flip_exit->sample(time,pose);c.flip_exit->sample_board(time,board);
            pose[0]=pose[1]=Joint{};
            auto unplaced=seed;unplaced[0]=unplaced[1]=Joint{};
            const auto shape=board_geometry::from_pose(unplaced,Joint{});
            float top=board.position[1];
            for(const auto& point:shape.points)top=std::max(top,board.position[1]+rotate(board.rotation,point)[1]);
            for(auto foot:{front,back})check(world(pose,foot).position[1]-top>.06f,
                "released shoes clear the rotating deck and trucks in mid-flip");
        }
        check(c.flip_exit->weight(.6f)==0 && c.direct_exit->weight(.3f)==0,"both exits release to the live native pose");
        const auto joined_exit=joined(*c.direct_exit,*c.entry_hold,1.25f);
        c.entry_hold->sample(1.25f,pose);joined_exit.sample(0,center);
        for(std::size_t j=0;j<pose.size();++j)check(same(pose[j],center[j]),"exit begins at the current balance pose without a jump");
        const auto twice=reversed(reversed(*c.direct_exit));
        check(twice.weights==c.direct_exit->weights,"reversing twice restores the blend envelope");
    }
}
void low_stance(){
    // A held Casper is a low inverted deck with separated feet, not a raised
    // front knee and a near-vertical board. Incoming Manual pitch must not set
    // the held inclination; both feet remain close to the support surface.
    for(bool goofy:{false,true})for(float pitch:{-12.f,0.f,12.f,24.f}) {
        const auto seed=flatland_pose_fixture(goofy);
        Joint native_board;native_board.rotation=from_euler({pitch,0,0});
        const auto clips=make_casper_clips(seed,native_board);
        for(float time:{1.f,1.5f,2.f,2.5f,3.f}) {
            std::vector<Joint> pose(skeleton_joints);Joint board;
            clips.entry_hold->sample(time,pose);clips.entry_hold->sample_board(time,board);
            const auto along=rotate(board.rotation,{0,0,1});
            check(along[1]>.10f && along[1]<.31f,"held deck remains low, with nose slightly above tail");
            check(rotate(board.rotation,{0,1,0})[1]<-.94f,"inverted deck stays nearly parallel to the surface");
            const float floor=world(seed,375).position[1];
            for(auto foot:{10u,343u}) {
                const auto ankle=world(pose,static_cast<std::uint16_t>(foot));
                check(ankle.position[1]>floor && ankle.position[1]<floor+.25f,"both ankles remain near the low deck");
            }
            check(std::abs(world(pose,10).position[1]-world(pose,343).position[1])<.14f,
                "front foot supports the low deck without a high-knee stance");
        }
    }
}
void raised_shoulders(){
    // A Manual can lift one clavicle. Its vertical offset must not become an
    // upward hand target, and moving the arm must not twist a fixed-world wrist.
    for(bool goofy:{false,true}) {
        auto seed=flatland_pose_fixture(goofy);
        seed[275].position[1]=.16f;seed[46].position[1]=.07f;
        Joint native_board;native_board.rotation=from_euler({6,0,0});
        const auto clips=make_casper_clips(seed,native_board);
        for(float time:{1.f,2.f,3.f}) {
            std::vector<Joint> pose(skeleton_joints);clips.entry_hold->sample(time,pose);
            for(const auto arm:{std::array<std::uint16_t,3>{47,48,49},std::array<std::uint16_t,3>{276,277,278}}) {
                check(world(pose,arm[2]).position[1]<world(pose,arm[0]).position[1]-.10f,
                    "raised clavicles retain a low balanced hand target");
                check(world(pose,arm[1]).position[1]<world(pose,arm[0]).position[1],
                    "balanced elbows bend below their shoulders");
                check(same(pose[arm[2]],seed[arm[2]]),"wrists follow the forearms without added flexion");
            }
        }
    }
}
void tall_manual_support(){
    for(bool goofy:{false,true}) {
        auto seed=flatland_pose_fixture(goofy);seed[hips_joint].position[1]+=.12f;
        Joint native_board;native_board.position[2]=.20f;
        const auto clips=make_casper_clips(seed,native_board);
        const auto front=static_cast<std::uint16_t>(goofy?10:343),back=static_cast<std::uint16_t>(goofy?343:10);
        const float side=goofy?-1.f:1.f;
        for(float time:{1.f,1.5f,2.f,2.5f,3.f}) {
            std::vector<Joint> pose(skeleton_joints);Joint board;
            clips.entry_hold->sample(time,pose);clips.entry_hold->sample_board(time,board);
            const auto toe=unpoint(board,world(pose,static_cast<std::uint16_t>(front+1)).position);
            check(toe[1]>.025f && toe[1]<.065f && std::abs(toe[0])>.115f && std::abs(toe[0])<.145f,
                "a tall rear-weighted Manual still reaches the low forefoot support");
            check(length(sub(world(pose,back).position,point(board,{-side*.015f,-.10f,-.32f})))<.015f,
                "settling the pelvis preserves rear foot support");
            check(seed[hips_joint].position[1]-pose[hips_joint].position[1]<=.201f,
                "support settling is bounded and never stretches leg bones");
        }
    }
}
void front_ankle_clearance(){
    // The under-board support belongs to the forefoot. Putting the ankle there
    // makes the lower leg pass through the deck, even when an IK target is met.
    for(bool goofy:{false,true}) {
        const auto clips=make_casper_clips(flatland_pose_fixture(goofy),{});
        const auto ankle=static_cast<std::uint16_t>(goofy?10:343);
        for(float time:{1.f,1.5f,2.f,2.5f,3.f}) {
            std::vector<Joint> pose(skeleton_joints);Joint board;
            clips.entry_hold->sample(time,pose);clips.entry_hold->sample_board(time,board);
            const auto a=unpoint(board,world(pose,ankle).position);
            const auto knee=unpoint(board,world(pose,static_cast<std::uint16_t>(ankle-1)).position);
            check(std::abs(a[0])>.15f,"front ankle clears the deck rail instead of entering the deck");
            if(a[1]*knee[1]<=0 && std::abs(knee[1]-a[1])>.0001f) {
                const auto crossing=add(a,scaled(sub(knee,a),-a[1]/(knee[1]-a[1])));
                check(std::abs(crossing[0])>.15f || std::abs(crossing[2])>.45f,
                    "lower leg crosses the deck plane outside its silhouette with clearance");
            }
        }
    }
}
void shoe_clearance(){
    // Independent volume proxy, not equality with the author's IK target.
    // Sample the lower leg, instep and toe end against the deck slab throughout
    // the baked path. Rendered shoe meshes still require separate game review.
    const auto slab_distance=[](const Vec3& point) {
        const Vec3 half{.105f,.012f,.4f};float squared=0;
        for(unsigned i=0;i<3;++i){const float d=std::max(0.f,std::abs(point[i])-half[i]);squared+=d*d;}
        return std::sqrt(squared);
    };
    for(bool goofy:{false,true}) {
        auto seed=flatland_pose_fixture(goofy);seed[hips_joint].position[1]+=.05f;
        const auto clips=make_casper_clips(seed,{});
        const auto front=static_cast<std::uint16_t>(goofy?10:343);
        for(const auto& clip:{clips.entry_hold,clips.direct_exit,clips.flip_exit}) {
            const float duration=clip==clips.entry_hold?.3f:clip->duration();
            for(unsigned frame=0;frame<=static_cast<unsigned>(duration*240);++frame) {
                const float time=static_cast<float>(frame)/240;
                std::vector<Joint> pose(skeleton_joints);Joint board;
                clip->sample(time,pose);clip->sample_board(time,board);
                const auto ankle=world(pose,front).position;
                const auto toe=world(pose,static_cast<std::uint16_t>(front+1)).position;
                auto direction=sub(toe,ankle);direction[1]=0;
                const std::array<Vec3,4> line{world(pose,static_cast<std::uint16_t>(front-1)).position,
                    ankle,toe,add(toe,scaled(unit(direction,{1,0,0}),.06f))};
                for(unsigned segment=0;segment<3;++segment)for(unsigned i=0;i<=96;++i) {
                    const auto at=add(line[segment],scaled(sub(line[segment+1],line[segment]),static_cast<float>(i)/96));
                    const float radius=segment==0?.045f:.03f;
                    check(slab_distance(unpoint(board,at))>=radius,
                        "lower leg and shoe envelope clear the deck throughout entry, flip and reversed exit");
                }
            }
        }
    }
}
void contact_anchor(){
    Joint frame;frame.position={3,2,1};frame.rotation=from_euler({17,43,-12});frame.scale={.9f,1.1f,1.2f};
    const Vec3 anchor{.12f,-.06f,.01f},target{-1,.2f,4};
    const auto placed=anchored(frame,anchor,target);
    check(length(sub(point(placed,anchor),target))<.00001f,"contact anchor accounts for rotation and scale");
    check(placed.rotation==frame.rotation && placed.scale==frame.scale,"contact anchoring preserves orientation and scale");
    frame.position[0]=std::numeric_limits<float>::quiet_NaN();rejected([&]{(void)anchored(frame,anchor,target);});
}
void range_motion(){
    RangeMotion still{},moving{.08f,.16f};
    for(float b:{-1.f,-.8f,0.f,.8f,1.f})for(unsigned i=0;i<=240;++i) {
        const float time=static_cast<float>(i)/60;
        check(still.sample(b,time)==b,"unconfigured ranges retain static sampling");
        const float at=moving.sample(b,time);
        check(at>=-1 && at<=1 && std::abs(at-b)<=.161f,"range motion remains a bounded correction around native balance");
        check(std::abs(at-moving.sample(b,time+RangeMotion::period))<.00001f,"wrapping native time keeps motion continuous");
    }
    check(moving.sample(0,.25f)>.05f,"neutral balance has a slow visible weight shift");
    const auto clips=make_casper_clips(flatland_pose_fixture(),{});
    std::vector<Joint> left(skeleton_joints),right(skeleton_joints);
    clips.entry_hold->sample(2+moving.sample(0,1.f/3),left);
    clips.entry_hold->sample(2+moving.sample(0,1.f),right);
    check(length(sub(world(left,49).position,world(right,49).position))>.012f,
        "quiet range motion moves the rendered hand rather than disappearing in a center easing plateau");
    check(moving.sample(std::numeric_limits<float>::quiet_NaN(),0)==0,"invalid range input fails closed");
}
void visible_motion(){
    for(bool goofy:{false,true}) {
        const auto clips=make_casper_clips(flatland_pose_fixture(goofy),{});
        float yaw=0,previous=0,roll=0,previous_roll=0;
        for(unsigned i=0;i<=144;++i) {
            Joint board;clips.flip_exit->sample_board(static_cast<float>(i)/240,board);
            const auto nose=rotate(board.rotation,{0,0,1});
            const float angle=std::atan2(nose[0],nose[2]);
            const float pitch=std::atan2(-nose[1],std::hypot(nose[0],nose[2]));
            const auto upright=rotate(multiply(conjugate(axis_angle({1,0,0},pitch)),
                conjugate(axis_angle({0,1,0},angle))),rotate(board.rotation,{0,1,0}));
            const float bank=std::atan2(-upright[0],upright[1]);
            if(i){yaw+=std::remainder(angle-previous,6.28318530718f);
                roll+=std::remainder(bank-previous_roll,6.28318530718f);}
            previous=angle;previous_roll=bank;
        }
        check(std::abs(std::abs(yaw)-6.28318530718f)<.05f,
            "flip completes a full horizontal board revolution as well as its roll");
        check(std::abs(std::abs(roll)-9.42477796f)<.05f,
            "horizontal spin retains the inverted-to-upright roll path");
        std::vector<Joint> a(skeleton_joints),b(skeleton_joints);
        clips.entry_hold->sample(2+clips.range_motion.sample(0,1.f/3),a);
        clips.entry_hold->sample(2+clips.range_motion.sample(0,1),b);
        check(length(sub(world(a,hips_joint).position,world(b,hips_joint).position))>.035f,
            "quiet hold visibly transfers the hips while native balance stays centered");
        for(auto joint:{49u,278u})check(length(sub(world(a,static_cast<std::uint16_t>(joint)).position,
                world(b,static_cast<std::uint16_t>(joint)).position))>.06f,
            "quiet hold moves both hands by a readable amount");
    }
}
void validation(){
    auto seed=flatland_pose_fixture();const auto c=make_casper_clips(seed,{});
    auto clip=*c.direct_exit;
    clip.weights[0]=std::numeric_limits<float>::quiet_NaN();rejected([&]{(void)clip_bytes(clip);});
    clip=*c.direct_exit;clip.joint_weights.pop_back();rejected([&]{(void)clip_bytes(clip);});
    clip=*c.direct_exit;clip.frames.pop_back();rejected([&]{(void)clip_bytes(clip);});
    auto bytes=clip_bytes(*c.direct_exit);bytes.pop_back();rejected([&]{(void)clip_from_bytes(bytes);});
    bytes=clip_bytes(*c.direct_exit);bytes[21+c.direct_exit->times.size()*4+3]=0x7f;
    rejected([&]{(void)clip_from_bytes(bytes);});
    clip=*c.direct_exit;clip.weights.clear();clip.joint_weights.clear();
    check(clip_bytes(clip)[3]=='3',"unweighted clips retain the established format");
    check(clip_from_bytes(clip_bytes(clip)).weight(.1f)==1,"old clips have full default weight");
    seed[9].position={};rejected([&]{(void)make_casper_clips(seed,{});});
    Phase invalid{"invalid",{{0,flatland_pose_fixture(),{},1},{1e9f,flatland_pose_fixture(),{},1}},{}};
    rejected([&]{(void)bake(invalid);});
}
}
int main(){visible_motion();contact_anchor();shoe_clearance();front_ankle_clearance();low_stance();clips();raised_shoulders();tall_manual_support();range_motion();validation();std::cout<<checks<<" authored motion checks passed (synthetic rig; not native contact acceptance)\n";}
