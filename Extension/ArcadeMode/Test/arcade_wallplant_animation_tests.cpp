#include "Extension/ArcadeMode/arcade_wallplant_animation_measurement.h"
#include "Extension/Skater/owned_playback_hold.h"
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
}
int main() {
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
    const auto direction=hold_wall_direction({0,0,1},{0,0,1});
    check(direction && *direction==std::array<float,3>{0,0,-1},"wall direction expressed behind outgoing facing");
    const auto angled=hold_wall_direction({.6f,0,.8f},{-.8f,0,.6f});
    check(angled && near((*angled)[0],-1)&&near((*angled)[2],0),"angled target converts real wall into local side");
    check(!hold_wall_direction({0,0,1},{0,0,0}),"missing outgoing basis cannot select pose");
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
