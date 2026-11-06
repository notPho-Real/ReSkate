#include "Extension/ArcadeMode/arcade_wallplant.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}
}
bool near(float a,float b){return std::abs(a-b)<.0001f;}
struct Fixture {
    WallplantPolicy policy;
    WallplantFrame frame{};
    WallplantContact contact{};
    WallplantPress press{};
    Fixture() {
        frame.now=1000;frame.velocity={-12,-2,0};frame.ground_clearance=2;
        frame.enabled=frame.focused=frame.controller_owned=frame.takeoff_observed=frame.clearance_known=true;
        frame.native.valid=true;frame.native.owner={1,2,3,4,5};frame.native.owner_generation=1;
        frame.native.contact_generation=2;frame.native.action_generation=3;frame.native.physics_step=20;
        frame.native.sampled_at=1000;frame.native.airborne=true;frame.native.airborne_episode=1;
        frame.native.airborne_started_known=true;frame.native.airborne_started_at=800;
        frame.geometry.owner={1,2,3,4,5};frame.geometry.client_context=6;frame.geometry.world=7;
        frame.geometry.world_generation=1;frame.geometry.self_entity=2;frame.geometry.self_generation=1;
        frame.geometry.phase=SurfaceGeometryPhase::client;
        contact.body={7,10,1};contact.point={0,2,0};contact.normal={1,0,0};contact.identity=1;
        contact.collision=contact.static_world=true;press.sequence=1;press.fresh_a=true;
        sync();
    }
    void sync() {
        auto& n=frame.geometry.native;n.required=n.valid=true;n.generations={frame.native.owner_generation,
            frame.native.contact_generation,frame.native.action_generation};
        n.physics_step=frame.native.physics_step;n.sampled_at=frame.native.sampled_at;
        n.airborne=frame.native.airborne;n.airborne_episode=frame.native.airborne_episode;n.air_departure=frame.native.air_departure;
        contact.native=frame.native;contact.geometry=frame.geometry;contact.sampled_at=frame.now;
        contact.approach_velocity=frame.velocity;press.native=n;press.pressed_at=frame.now;
    }
    WallplantCommand run(){return policy.step(frame,contact,press);}
    void advance(std::uint64_t amount) {
        frame.now+=amount;frame.native.sampled_at=frame.now;++frame.native.physics_step;sync();
    }
};
}
int main() {
    for(unsigned rate:{30u,60u,120u})for(float side:{-1.0f,1.0f})for(float tangent:{0.0f,7.0f}) {
        Fixture f;f.frame.native.physics_seconds=1.0f/static_cast<float>(rate);
        f.contact.normal={side,0,0};f.frame.velocity={-12*side,-2,tangent};f.sync();
        const auto command=f.run();check(bool(command),"head-on and angled collision admit in both directions and cadences");
        check(command.after[0]*side>0 && near(command.after[2],tangent),"one rebound reverses normal and preserves tangent");
        check(near(command.after[1],500*GameplayProfile::metres_per_inch),"source-backed vertical exit target");
        check(f.policy.current(command,f.frame,f.contact),"command retains exact current native transaction identity");
        std::array<SurfaceGeometryVector,2> velocities{f.frame.velocity,SurfaceGeometryVector{f.frame.velocity[0]+.2f,-1,tangent+.3f}};
        std::array<SurfaceGeometryVector,2> spins{SurfaceGeometryVector{1,2,3},SurfaceGeometryVector{2,3,4}};
        const auto old_spins=spins;check(WallplantPolicy::apply_bodies(command,velocities,spins),"finite articulated batch applies");
        check(near(velocities[1][0]-velocities[0][0],.2f) && near(velocities[1][1]-velocities[0][1],1),"relative body velocity survives kick");
        check(spins==old_spins,"rebound does not steal native spin");
        check(!WallplantPolicy::apply_bodies(command,velocities,spins),"already-applied root cannot repeat same delta");
        check(!f.run(),"same input cannot produce another occurrence");
        ++f.press.sequence;check(!f.run(),"fresh input on unresolved contact cannot retrigger");
    }
    for(unsigned fault=0;fault<41;++fault) {
        Fixture f;
        switch(fault) {
        case 0:f.frame.enabled=false;break;
        case 1:f.frame.focused=false;break;
        case 2:f.frame.controller_owned=false;break;
        case 3:f.frame.competing_owner=true;break;
        case 4:f.frame.takeoff_observed=false;break;
        case 5:f.frame.clearance_known=false;break;
        case 6:f.frame.ground_clearance=.1f;break;
        case 7:f.frame.native.valid=false;break;
        case 8:f.frame.native.airborne=false;break;
        case 9:f.frame.native.rail_family=true;break;
        case 10:f.frame.native.airborne_started_known=false;break;
        case 11:f.frame.now+=151;break;
        case 12:f.press.fresh_a=false;break;
        case 13:f.press.sequence=0;break;
        case 14:f.press.pressed_at=1001;break;
        case 15:f.press.pressed_at=799;break;
        case 16:++f.press.native.generations.owner;break;
        case 17:++f.press.native.airborne_episode;break;
        case 18:f.contact.collision=false;break;
        case 19:f.contact.static_world=false;break;
        case 20:f.contact.rail=true;break;
        case 21:f.contact.skater=true;break;
        case 22:f.contact.normal={0,1,0};break;
        case 23:f.contact.normal={0,-1,0};break;
        case 24:f.contact.normal={.8f,.6f,0};break;
        case 25:f.contact.normal={0,0,0};break;
        case 26:f.contact.normal={2,0,0};break;
        case 27:f.contact.point[0]=std::numeric_limits<float>::quiet_NaN();break;
        case 28:f.contact.sampled_at=849;break;
        case 29:++f.contact.geometry.world_generation;break;
        case 30:++f.contact.native.owner_generation;break;
        case 31:f.contact.body.world=99;break;
        case 32:f.contact.approach_velocity={12,0,0};break;
        case 33:f.frame.velocity={1,-2,0};break;
        case 34:f.contact.approach_velocity={-1,0,20};break;
        case 35:f.contact.native.sampled_at=849;break;
        case 36:++f.frame.geometry.owner[0];break;
        case 37:++f.frame.geometry.native.generations.action;break;
        case 38:f.contact.geometry.self_generation=0;break;
        case 39:f.contact.body.index=UINT32_MAX;break;
        case 40:f.contact.approach_velocity={0,0,3};break;
        }
        check(!f.run(),"missing ownership, evidence, input or inappropriate geometry fail closed");
        f.frame=Fixture{}.frame;f.contact=Fixture{}.contact;f.press=Fixture{}.press;
        if(fault!=13)check(!f.run(),"rejected physical edge cannot be promoted by later readiness");
    }
    {
        Fixture f;f.frame.velocity={0,-2,0};
        check(bool(f.run()),"fresh genuine incoming collision still works after native response stops inward motion");
        f.policy.cancel();check(!f.run(),"cancellation cannot replay an old physical edge");
        f.policy.reset();check(bool(f.run()),"explicit journal/session reset restarts its sequence namespace");
        f.policy.reset();f.contact.body.generation=0;
        check(bool(f.run()),"allocated native shape generation zero is a valid identity");
    }
    {
        Fixture f;const auto first=f.run();check(bool(first),"first wall executes");
        f.advance(1100);++f.contact.identity;++f.press.sequence;
        check(!f.run(),"time and changed contact identity alone cannot prove separation");
        f.frame.separated=true;f.press.fresh_a=false;f.run();f.frame.separated=false;
        f.advance(17);++f.contact.identity;++f.press.sequence;f.press.fresh_a=true;
        f.contact.body.index=11;f.contact.normal={-1,0,0};f.frame.velocity={12,-1,3};f.sync();
        check(bool(f.run()),"different wall with separation, new approach and fresh edge executes");
    }
    {
        Fixture f;f.run();f.advance(17);f.frame.separated=true;f.press.fresh_a=false;f.run();
        f.frame.separated=false;f.advance(17);++f.press.sequence;++f.contact.identity;f.press.fresh_a=true;
        check(!f.run(),"concave corner cannot bypass source-backed repeat bound");
    }
    {
        Fixture f;auto command=f.run();f.frame.velocity[0]+=1;
        check(!f.policy.current(command,f.frame,f.contact),"native velocity takeover invalidates cached command");
        f.frame.velocity=command.before;++f.contact.body.generation;
        check(!f.policy.current(command,f.frame,f.contact),"body generation changed before commit invalidates command");
        std::array<SurfaceGeometryVector,2> velocities{command.before,SurfaceGeometryVector{0,0,std::numeric_limits<float>::infinity()}};
        std::array<SurfaceGeometryVector,2> spins{};const auto old=velocities;
        check(!WallplantPolicy::apply_bodies(command,velocities,spins) && velocities==old,"one invalid body rejects batch without partial writes");
    }
    {
        Fixture f;f.frame.velocity={-900,-2,10};f.sync();auto command=f.run();
        check(bool(command) && std::hypot(command.after[0],command.after[2])<=GameplayProfile{}.hard_speed()+.0001f,
              "large inward speed cannot accumulate above existing horizontal cap");
        check(near(command.after[2],10),"bounded normal exit still preserves useful tangent");
        WallplantParameters invalid=WallplantParameters::reference_prototype();invalid.vertical_exit_speed=std::numeric_limits<float>::quiet_NaN();
        f.policy=WallplantPolicy(invalid);check(!f.run(),"invalid tuning never emits velocity");
    }
    std::cout<<checks<<" wallplant policy checks passed; synthetic observations are not hardware acceptance\n";
}
