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
            check(length(sub(a.position,point(board,{side*.025f,.025f,.25f})))<.015f,"front instep stays at its authored board target across balance");
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
int main(){clips();raised_shoulders();validation();std::cout<<checks<<" authored motion checks passed (synthetic rig; not native contact acceptance)\n";}
