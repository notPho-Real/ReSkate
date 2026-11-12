#include "Extension/ArcadeMode/arcade_wallplant_animation_measurement.h"
#include "Extension/ArcadeMode/arcade_wallplant_animation_assets.h"
#include "Extension/ArcadeMode/arcade_wallplant_animation_contact.h"
#include "Extension/Skater/owned_playback_hold.h"
#include "Extension/Skater/body_contact.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode::wallplant_animation;
namespace {
unsigned checks{};
void check(bool value,const char* label) {
    ++checks;if(!value){std::cerr<<label<<'\n';std::exit(1);}
}
bool near(float a,float b){return std::abs(a-b)<.0001f;}
struct PhaseLibrary {
    mutable unsigned reads{}, mirrors{};
    bool empty{}, fail_mirror{}, missing_ground{};
    std::string_view joint_name(std::uint16_t joint) const {return !missing_ground && joint==0?"Ground":"";}
    dingosdk::anim_studio::Clip load(std::string_view name) const {
        ++reads;
        // Absorb/trick-out assets are deliberately unavailable: a held play
        // must resolve the native sequence's separate stationary asset.
        if(name!="animation/dingo/c_proto_onb_grab_bs_1ft_judo_footplant_loop")
            throw std::runtime_error("not the native held phase");
        dingosdk::anim_studio::Clip clip;
        if(!empty){clip.joints=1;clip.times={0,305.f/60.f};clip.frames.resize(2);clip.frames[0].position[0]=1;}
        return clip;
    }
    dingosdk::anim_studio::Clip mirror(dingosdk::anim_studio::Clip clip) const {
        ++mirrors;if(fail_mirror)throw std::runtime_error("mirror unavailable");
        clip.frames[0].position[0]*=-1;return clip;
    }
};
}
int main() {
    using namespace dingosdk;
    for(const auto foot:{bonecam::joint::left_foot,bonecam::joint::right_foot}) {
        std::array<anim_studio::Joint,anim_studio::skeleton_joints> native;
        native[1].position={3,2,-4};
        native[1].rotation=anim_studio::from_euler({35,50,10});
        const auto knee=static_cast<std::uint16_t>(anim_studio::parent(foot));
        const auto thigh=static_cast<std::uint16_t>(anim_studio::parent(knee));
        native[knee].position={0,-.4f,0};native[foot].position={0,-.4f,0};
        native[thigh].rotation=anim_studio::from_euler({110,0,0});
        anim_studio::Clip clip;
        clip.joints=anim_studio::skeleton_joints;clip.times={0};
        clip.frames.assign(native.begin(),native.end());clip.turned_only.resize(clip.joints,1);
        clip.frames[thigh].rotation=anim_studio::from_euler({10,0,0});
        const arm_reach::ContactFrame frame{{0,1,0},.05f};
        const anim_studio::Vec3 point{3,2,-5},normal{0,0,1};
        auto full=native;
        skater_animation::blend_body_pose(full,clip.frame(0),clip,1);
        const auto expected=arm_reach::contact_target(anim_studio::world(full,foot),frame,point,normal);
        check(expected.has_value(),"Full authored endpoint supplies a valid surface anchor");
        for(const float blend:{0.f,.25f,.5f,.75f,1.f}) {
            auto drawn=native;
            skater_animation::blend_body_pose(drawn,clip.frame(0),clip,blend);
            const auto before=drawn;
            const auto target=skater_animation::held_contact_target(drawn,clip,0,foot,frame,point,normal);
            check(target.has_value(),"Entry blend admits the same authored contact");
            for(unsigned k=0;k<3;++k)
                check(near(target->position[k],expected->position[k]),"First render weight cannot pin the incoming crouch endpoint");
            for(unsigned k=0;k<4;++k)
                check(near(target->rotation[k],expected->rotation[k]),"First render weight cannot pin the incoming foot twist");
            check(drawn[1].rotation==before[1].rotation && drawn[foot].position==before[foot].position,
                "Anchor selection does not alter placement or native limb length");
        }
        check(!skater_animation::held_contact_target(native,clip,1,foot,frame,point,normal),"Out-of-clip held time refuses anchoring");
        check(!skater_animation::held_contact_target(native,clip,0,anim_studio::hips_joint,frame,point,normal),"Non-foot endpoint refuses anchoring");
    }
    for(const auto foot:{bonecam::joint::left_foot,bonecam::joint::right_foot}) {
        std::vector<anim_studio::Joint> pose(anim_studio::skeleton_joints);
        pose[1].position={3,2,-4};
        pose[1].rotation=anim_studio::from_euler({35,50,10});
        for(const auto tip:{bonecam::joint::left_foot,bonecam::joint::right_foot}) {
            const auto knee=static_cast<std::uint16_t>(anim_studio::parent(tip));
            const auto thigh=static_cast<std::uint16_t>(anim_studio::parent(knee));
            pose[thigh].position={tip==foot?.12f:-.12f,0,0};
            pose[knee].position={0,-.4f,.2f};pose[tip].position={0,-.25f,-.2f};
        }
        const auto before=pose;
        const auto original_hips=anim_studio::world(pose,anim_studio::hips_joint);
        const auto tip=anim_studio::world(pose,foot);
        const auto other=foot==bonecam::joint::left_foot?bonecam::joint::right_foot:bonecam::joint::left_foot;
        const auto board=anim_studio::world(pose,other);
        auto levelled=pose;
        check(skater_animation::level_contact_pose(levelled,foot),"Contact preparation retains a reachable board foot");
        const auto levelled_tip=anim_studio::world(levelled,foot);
        const arm_reach::ContactFrame surface{{0,1,0},0};
        const anim_studio::Vec3 wall_normal{0,0,1};
        anim_studio::Clip held;
        held.joints=anim_studio::skeleton_joints;held.times={0};
        held.frames=pose;held.turned_only.resize(held.joints,1);
        held.frames[2].position={0,-1,0};
        const auto direction=hold_wall_direction(wall_normal,pose[1].rotation);
        check(direction.has_value(),"Banked fixture has a measured wall direction");
        const auto selection=plant_contact(held,2,*direction,pose,true);
        check(selection.has_value(),"Levelled selection admits an authored support foot");
        auto selected_pose=pose;
        check(skater_animation::level_contact_pose(selected_pose,selection->sample.joint),
            "Selected contact uses the same reachable levelled frame");
        const auto selected_tip=anim_studio::world(selected_pose,selection->sample.joint);
        const auto selected_relative=anim_studio::rotate(anim_studio::conjugate(pose[1].rotation),
            {selected_tip.position[0]-pose[1].position[0],selected_tip.position[1]-pose[1].position[1],
             selected_tip.position[2]-pose[1].position[2]});
        float expected_reach{};
        for(unsigned k=0;k<3;++k)expected_reach+=selected_relative[k]*(*direction)[k];
        check(near(selection->sample.reach,expected_reach),"Selection ranks the drawn frame rather than inherited bank");
        const auto anchor=skater_animation::held_contact_target(pose,held,0,foot,surface,
            levelled_tip.position,wall_normal,true);
        check(anchor.has_value(),"A levelled hold derives its anchor in the frame that will be drawn");
        check(std::abs(tip.position[1]-levelled_tip.position[1])>.01f,
            "Banked regression separates old and levelled endpoint heights");
        for(unsigned k=0;k<3;++k)
            check(near(anchor->position[k],levelled_tip.position[k]),
                "Contact anchor does not pull a levelled leg back to the banked endpoint");
        auto anchored=pose;
        check(skater_animation::level_body_contact(anchored,foot,anchor->position,anchor->rotation,1),
            "Prepared anchor is reachable in the drawn body frame");
        std::optional<arm_reach::Placed> held_anchor;
        auto early=pose;
        early[anim_studio::placement_joint].position[0]+=2;
        early[anim_studio::placement_joint].position[2]+=2;
        const auto early_before=early;
        const auto rejected_target=skater_animation::held_contact_target(early,held,0,foot,surface,
            levelled_tip.position,wall_normal,true);
        check(rejected_target.has_value(),"An unreachable projection still produces a candidate endpoint");
        std::optional<arm_reach::Placed> attempted;
        check(!skater_animation::blend_held_surface_contact(early,held,0,foot,surface,
            levelled_tip.position,wall_normal,1,held_anchor,&attempted),"Unreachable first draw refuses the held contact");
        check(!held_anchor,"Refused first draw does not retain its candidate anchor");
        check(attempted && attempted->position==rejected_target->position,
            "Refused candidate remains available for draw measurements without becoming the anchor");
        for(std::size_t j=0;j<early.size();++j)
            check(early[j].position==early_before[j].position && early[j].rotation==early_before[j].rotation,
                "Refused first draw preserves the native pose");
        auto later=pose;
        check(!skater_animation::blend_held_contact(later,held,0,foot,rejected_target->position,rejected_target->rotation,1),
            "Retaining the refused endpoint would reject the later reachable placement");
        check(skater_animation::blend_held_surface_contact(later,held,0,foot,surface,
            levelled_tip.position,wall_normal,1,held_anchor),"A later reachable placement establishes the held contact");
        check(held_anchor.has_value(),"Successful solve commits the anchor");
        const auto fixed=*held_anchor;
        auto moved=pose;
        moved[anim_studio::placement_joint].position[0]+=2;
        check(!skater_animation::blend_held_surface_contact(moved,held,0,foot,surface,
            levelled_tip.position,wall_normal,1,held_anchor),"Established anchor cannot slide to follow later placement");
        check(held_anchor->position==fixed.position && held_anchor->rotation==fixed.rotation,
            "A later refusal preserves the established wall anchor");
        // The native incoming hips can be somewhere else even though the
        // authored held sample and its wall contact are reachable.
        auto incoming=before;
        incoming[anim_studio::hips_joint].position={0,1.5f,0};
        held.turned_only[anim_studio::hips_joint]=0;
        held.frames[anim_studio::hips_joint].position=before[anim_studio::hips_joint].position;
        auto premature=incoming;
        check(!skater_animation::level_body_contact(premature,foot,anchor->position,anchor->rotation,1),
            "Incoming pose cannot reach the held pose anchor before authored hips are applied");
        for(const float weight:{0.f,.25f,.5f,.75f,1.f}) {
            auto drawn=incoming;
            check(skater_animation::blend_held_contact(drawn,held,0,foot,anchor->position,anchor->rotation,weight),
                "Full authored contact is solved before the entry blend");
            auto expected=incoming;
            skater_animation::blend_body_pose(expected,anchored,held,weight);
            for(std::size_t j=0;j<drawn.size();++j) {
                for(unsigned k=0;k<3;++k)
                    check(near(drawn[j].position[k],expected[j].position[k]),"Held contact blends authored translation once");
                for(unsigned k=0;k<4;++k)
                    check(near(drawn[j].rotation[k],expected[j].rotation[k]),"Held contact blends solved rotation once");
                check(drawn[j].scale==incoming[j].scale,"Held contact does not change body scale");
            }
        }
        auto failed=incoming;
        check(!skater_animation::blend_held_contact(failed,held,0,foot,{100,100,100},anchor->rotation,1),
            "Unreachable full authored contact still refuses atomically");
        for(std::size_t j=0;j<failed.size();++j)
            check(failed[j].position==incoming[j].position && failed[j].rotation==incoming[j].rotation,
                "Failed authored contact preserves the entire incoming pose");
        check(!plant_contact(held,2,*direction,pose,true,ContactSurface{{100,100,100},wall_normal}),
            "Directional reach alone cannot select a pose for an unreachable actual wall");
        for(unsigned k=0;k<3;++k) {
            check(near(anim_studio::world(anchored,foot).position[k],levelled_tip.position[k]),
                "Drawn plant retains the levelled authored endpoint");
            check(near(anim_studio::world(anchored,other).position[k],board.position[k]),
                "Levelled anchoring retains the native board foot");
        }
        check(skater_animation::level_body_contact(pose,foot,tip.position,tip.rotation,1),
            "A banked authored body levels only when both existing foot endpoints remain reachable");
        const auto kept=anim_studio::world(pose,other),planted=anim_studio::world(pose,foot);
        for(unsigned k=0;k<3;++k) {
            check(near(kept.position[k],board.position[k]),"Board foot stays at its original endpoint");
            check(near(planted.position[k],tip.position[k]),"Plant foot stays at its observed endpoint");
        }
        const auto up=anim_studio::rotate(anim_studio::world(pose,anim_studio::hips_joint).rotation,{0,1,0});
        check(near(up[0],0)&&near(up[1],1)&&near(up[2],0),"Inherited placement bank and pitch are removed from the authored body frame");
        check(anim_studio::world(pose,anim_studio::hips_joint).position==original_hips.position,
            "Levelling pivots about the original hips without a render translation");
        for(std::size_t i=0;i<pose.size();++i)
            check(pose[i].position==before[i].position && pose[i].scale==before[i].scale,
                "Level contact never moves a joint origin locally or changes bone length");
        check(pose[0].rotation==before[0].rotation && pose[1].rotation==before[1].rotation,
            "World and native placement are unchanged");
        pose=before;
        check(!skater_animation::level_body_contact(pose,foot,{100,100,100},tip.rotation,1),
            "Unreachable plant refuses the entire correction");
        for(std::size_t i=0;i<pose.size();++i)
            check(pose[i].rotation==before[i].rotation,"Failed contact does not partially level the torso or move the other foot");
        check(!skater_animation::level_body_contact(pose,foot,tip.position,{0,0,0,0},1),
            "Invalid endpoint orientation refuses the whole correction");
        check(!skater_animation::level_body_contact(pose,foot,tip.position,tip.rotation,NAN),
            "Invalid blend cannot publish a body correction");
        check(skater_animation::level_body_contact(pose,foot,tip.position,tip.rotation,0),
            "Zero blend remains a valid bounded contact");
        for(std::size_t i=0;i<pose.size();++i)
            for(unsigned k=0;k<4;++k)check(near(pose[i].rotation[k],before[i].rotation[k]),
                "Zero blend retains the original pose");
    }
    for(const auto planted:{bonecam::joint::right_foot,bonecam::joint::left_foot}) {
        anim_studio::Clip clip;
        clip.joints=anim_studio::skeleton_joints;clip.times={0,.1f};
        clip.frames.resize(2*clip.joints);clip.turned_only.resize(clip.joints,1);
        for(std::size_t i=0;i<2;++i) {
            auto* pose=clip.frames.data()+i*clip.joints;
            // Ground is below the lower foot. The raised foot reaches farther
            // toward the wall but must not replace the authored planted foot.
            pose[2].position={0,-1,0};
            pose[planted].position={0,-.9f,0};
            const auto raised=planted==bonecam::joint::right_foot?bonecam::joint::left_foot:bonecam::joint::right_foot;
            pose[raised].position={0,-.4f,2};
        }
        std::vector<anim_studio::Joint> native(clip.frame(0).begin(),clip.frame(0).end());
        const auto selected=plant_contact(clip,2,{0,0,1},native);
        check(selected && selected->sample.joint==planted && near(selected->frame.clearance,.1f),
            "Both stances select the authored ground foot even when the tucked foot reaches farther toward the wall");
        check(selected && selected->sample.time==0,"Equal held geometry keeps the first sample");
        clip.frames[2].rotation={NAN,0,0,1};
        check(!plant_contact(clip,2,{0,0,1},native),"Broken authored plane refuses selection");
    }
    {
        std::vector<anim_studio::Joint> native(anim_studio::skeleton_joints);
        native[2].position={0,-1,0};
        for(const auto foot:{bonecam::joint::left_foot,bonecam::joint::right_foot}) {
            const auto knee=anim_studio::parent(foot),thigh=anim_studio::parent(static_cast<std::uint16_t>(knee));
            native[thigh].position={foot==bonecam::joint::right_foot?.12f:-.12f,0,0};
            native[knee].position={0,-.4f,.2f};native[foot].position={0,-.25f,-.2f};
        }
        anim_studio::Clip held;
        held.joints=anim_studio::skeleton_joints;held.times={0,.1f};
        held.frames=native;held.frames.insert(held.frames.end(),native.begin(),native.end());
        held.turned_only.resize(held.joints,1);held.turned_only[anim_studio::hips_joint]=0;
        held.frames[held.joints+anim_studio::hips_joint].position={0,0,-5};
        const auto unconstrained=plant_contact(held,2,{0,0,-1},native,true);
        check(unconstrained && near(unconstrained->sample.time,.1f),"Direction-only ranking prefers a pose beyond the real wall");
        const auto constrained=plant_contact(held,2,{0,0,-1},native,true,ContactSurface{{0,0,-.35f},{0,0,1}});
        check(constrained && constrained->sample.time==0,"Actual wall reach selects the feasible authored sample");
        check(!plant_contact(held,2,{0,0,-1},native,true,ContactSurface{{0,0,-50},{0,0,1}}),
            "No feasible held sample means no visual contact admission");
    }
    PhaseLibrary phases;
    const auto poses=load_held_poses(phases);
    check(phases.reads==1 && phases.mirrors==1,"both stances resolve one stationary native phase");
    check(poses.poses[0] && poses.poses[1] && poses.poses[0]->frames[0].position[0]==1 &&
        poses.poses[1]->frames[0].position[0]==-1,"held stance mirroring retains the original clip");
    PhaseLibrary missing;missing.empty=true;bool refused=false;
    try{(void)load_held_poses(missing);}catch(const std::runtime_error&){refused=true;}
    check(refused && !missing.mirrors,"missing held asset cannot fall back to absorb or release");
    PhaseLibrary unmirrored;unmirrored.fail_mirror=true;refused=false;
    try{(void)load_held_poses(unmirrored);}catch(const std::runtime_error&){refused=true;}
    check(refused,"mirror failure cannot publish a partial stance set");
    PhaseLibrary no_ground;no_ground.missing_ground=true;refused=false;
    try{(void)load_held_poses(no_ground);}catch(const std::runtime_error&){refused=true;}
    check(refused,"missing authored support plane cannot publish a contact clip");
    check(hold_remaining_ms(1000,1000)==160,"entry retains original160ms");
    check(hold_remaining_ms(1000,1080)==80,"delayed client gets only remaining hold time");
    check(hold_remaining_ms(1000,1159)==1,"last millisecond remains eligible");
    check(!hold_remaining_ms(1000,1160),"deadline cannot be renewed by client");
    check(!hold_remaining_ms(1000,999),"future start refused");
    check(!hold_remaining_ms(0,10),"missing physical start refused");
    check(hold_remaining_ms(UINT64_MAX-160,UINT64_MAX-1)==1,"clock comparison cannot overflow");
    check(!hold_remaining_ms(UINT64_MAX-160,UINT64_MAX),"overflow-edge deadline exact");
    using namespace dingosdk::skater_animation;
    check(owned_hold_deadline(1000,1160),"finite borrowed hold deadline admitted");
    check(!owned_hold_deadline(1000,1000)&&!owned_hold_deadline(1000,999),"expired hold refused");
    check(!owned_hold_deadline(1000,2001),"borrowed hold cannot be indefinite");
    check(near(owned_hold_weight(1159,1160,0,.04f),1),"authored pose stays held before deadline");
    check(near(owned_hold_weight(1180,1160,0,.04f),.5f),"callback fades after deadline without client update");
    check(near(owned_hold_weight(1200,1160,0,.04f),0),"callback fully restores native pose after bounded blend");
    check(near(owned_hold_weight(1120,1160,1100,.04f),.5f),"actual early release begins its own blend");
    check(near(owned_hold_weight(1180,1160,1190,.04f),.5f),"late client release cannot extend original deadline");
    check(owned_hold_release(7,7,7,1160,true,0,1100)==1100,"matching owned held ticket may release");
    check(owned_hold_release(7,7,7,1160,true,1100,1130)==1100,"repeated release does not renew blend");
    check(!owned_hold_release(8,7,7,1160,true,0,1100),"foreign requester cannot stop current borrower");
    check(!owned_hold_release(7,8,8,1160,true,0,1100),"stale ticket cannot stop replacement borrower");
    check(!owned_hold_release(7,7,8,1160,true,0,1100),"replaced serial cannot release");
    check(!owned_hold_release(7,7,7,0,true,0,1100),"ordinary pause is not a borrowed bounded hold");
    check(!owned_hold_release(7,7,7,1160,false,0,1100),"ordinary moving playback is unaffected");
    const auto direction=hold_wall_direction({0,0,1},{0,0,0,1});
    check(direction && *direction==std::array<float,3>{0,0,-1},"wall direction expressed in observed identity placement");
    const auto half=std::sqrt(.5f);
    const auto turned=hold_wall_direction({0,0,1},{0,half,0,half});
    check(turned && near((*turned)[0],1)&&near((*turned)[2],0),"observed yaw converts real wall into local side");
    const auto pitched=hold_wall_direction({0,0,1},{half,0,0,half});
    check(pitched && near((*pitched)[1],-1)&&near((*pitched)[2],0),"actual pitch changes wall-facing authored direction");
    const auto rolled=hold_wall_direction({1,0,0},{0,0,half,half});
    check(rolled && near((*rolled)[0],0)&&near((*rolled)[1],1),"actual roll is retained for side-wall selection");
    const auto antipodal=hold_wall_direction({0,0,1},{-half,0,0,-half});
    check(antipodal && pitched && *antipodal==*pitched,"quaternion sign does not change selection direction");
    check(!hold_wall_direction({0,0,1},{0,0,0,0}),"missing observed rotation cannot select pose");
    check(!hold_wall_direction({0,0,1},{0,0,0,2}),"nonunit observed rotation rejected");
    check(!hold_wall_direction({0,0,1},{0,0,0,std::numeric_limits<float>::quiet_NaN()}),"nonfinite observed rotation rejected");
    check(!hold_wall_direction({0,0,0},{0,0,0,1}),"missing wall normal rejected");
    std::optional<PlantFrame> pitched_selection;
    check(consider_plant_frame(pitched_selection,.1f,10,{0,0,-.8f},*pitched),"pitched body considers behind-frame candidate");
    check(consider_plant_frame(pitched_selection,.2f,343,{0,-.7f,0},*pitched),"pitched body considers actual wall-facing candidate");
    check(pitched_selection && pitched_selection->joint==343 && pitched_selection->time==.2f,
        "pitched placement chooses wall-facing foot instead of the upright-target winner");
    std::optional<PlantFrame> selected;
    check(consider_plant_frame(selected,.1f,10,{0,0,-.3f},*direction),"first authored native candidate measured");
    check(consider_plant_frame(selected,.4f,343,{0,0,-.8f},*direction),"later authored native extension measured");
    check(consider_plant_frame(selected,.5f,10,{0,0,1},*direction),"opposite-wall extension considered without selecting it");
    check(selected && selected->time==.4f && selected->joint==343 && near(selected->reach,.8f),"held frame chosen by actual wall-facing reach rather than guessed timestamp");
    check(consider_plant_frame(selected,.6f,10,{0,0,-.8f},*direction)&&selected->time==.4f,"equal reach keeps first authored frame deterministically");
    check(!consider_plant_frame(selected,.7f,10,{0,0,std::numeric_limits<float>::quiet_NaN()},*direction),"invalid clip transform cannot select frame");
    const std::array<float,3> point{3,2,-5},normal{0,0,1};
    auto sample=measure_foot_wall({6,6,-4.75f},point,normal);
    check(sample && near(sample->signed_plane_distance,.25f) && near(sample->tangent_distance,5),"separate plane and tangent distances");
    sample=measure_foot_wall({3,2,-5.3f},point,normal);
    check(sample && near(sample->signed_plane_distance,-.3f) && near(sample->tangent_distance,0),"penetration retains signed distance");
    sample=measure_foot_wall({4,2,-4},point,{.6f,0,.8f});
    check(sample && near(sample->signed_plane_distance,1.4f) && near(sample->tangent_distance,.2f),"angled plane measured without axis assumption");
    check(!measure_foot_wall(point,point,{0,0,0}),"missing normal refused");
    check(!measure_foot_wall(point,point,{0,0,2}),"nonunit normal refused");
    check(!measure_foot_wall({std::numeric_limits<float>::quiet_NaN(),0,0},point,normal),"invalid foot refused");
    check(!measure_foot_wall(point,{0,std::numeric_limits<float>::infinity(),0},normal),"invalid collision point refused");
    check(!measure_foot_wall(point,point,{0,0,std::numeric_limits<float>::infinity()}),"invalid normal refused");
    check(!measure_foot_wall({std::numeric_limits<float>::max(),0,0},point,normal),"unbounded distance refused");
    std::cout<<checks<<" wallplant visual timing/measurement checks passed\n";
}
