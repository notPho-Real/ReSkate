// Offline boundary fixture: includes the real runtime implementation unchanged.
#include "Extension/ArcadeMode/arcade_wallplant_runtime.cpp"
#include <cstdlib>
#include <iostream>
#include <thread>
namespace fixture {
std::uint64_t now=1000,query_latency{};
bool hit_current=true,race_cancel{};
bool collision_scope{};
bool geometry_mode{},motion_geometry{};
bool clearance_policy{};
unsigned clearance_queries{};
dingosdk::arcade_mode::SurfaceGeometrySample clearance_result{};
dingosdk::arcade_mode::SurfaceGeometryWorldOwner clearance_owner{};
dingosdk::arcade_mode::SurfaceGeometrySample wall{};
std::array<float,3> anchor_point{},anchor_offset{};
dingosdk::arcade_mode::native_wall_contact::Snapshot collision{};
unsigned begins{},releases{},cancels{},checks{},log_calls{};
std::uint64_t released_identity{};
std::uint64_t visual_expiry{};
dingosdk::arcade_mode::WallplantCommand active{};
std::array<float,3> visual_normal{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
}
std::uint64_t GetTickCount64(){return fixture::now;}
DWORD GetLastError(){return 0;}
void SetLastError(DWORD){}
namespace dingosdk::logging {
bool enabled(Level) noexcept{return true;}
void write(Level,Context,Channel,std::string_view) noexcept{++fixture::log_calls;}
void write(Level,Channel,std::string_view) noexcept{++fixture::log_calls;}
}
namespace dingosdk::arcade_mode::native_wall_contact {
bool start(std::uintptr_t) noexcept{return true;}
void enable(bool) noexcept{}
Snapshot observe(const NoBailSkater&,std::uint64_t) noexcept{return fixture::collision;}
bool collision_current(std::uint64_t serial,const NoBailSkater& owner) noexcept {
    return fixture::collision_scope && serial && serial==fixture::collision.serial && owner==fixture::collision.owner;
}
bool current(const Contact&,std::uintptr_t) noexcept{return fixture::hit_current;}
bool static_world_owner(const SurfaceGeometryBody& body,SurfaceGeometryWorldOwner& owner) noexcept {
    owner=fixture::clearance_policy && body==fixture::clearance_result.body?
        fixture::clearance_owner:fixture::wall.world_owner;
    return fixture::motion_geometry && fixture::hit_current;
}
bool local_component_owner(const SurfaceGeometryBody&,std::uintptr_t,SurfaceGeometryWorldOwner&) noexcept{return false;}
}
namespace dingosdk::arcade_mode::wallplant_animation {
void tick(bool) noexcept{}
bool available(const NoBailSkater&) noexcept{return true;}
void cancel() noexcept{++fixture::cancels;}
void release(const NoBailSkater&,std::uint64_t identity) noexcept{++fixture::releases;fixture::released_identity=identity;}
bool begin_hold(const NoBailSkater&,std::uint64_t,std::uint64_t,std::uint64_t expires,const std::array<float,3>&,
        const std::array<float,3>& normal,const std::array<float,3>&) noexcept {
    ++fixture::begins;fixture::visual_normal=normal;fixture::visual_expiry=expires;
    if(fixture::race_cancel)wallplant_runtime::finish_hold(fixture::active,false,fixture::now);
    return true;
}
}
namespace dingosdk::arcade_mode {
SurfaceGeometrySample query_surface_geometry(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext&,
        const SurfaceGeometryRequest&,SurfaceGeometryTrace*) noexcept {
    if(!fixture::clearance_policy)return {};
    ++fixture::clearance_queries;
    fixture::check(api.static_world_owner && api.local_component_owner,
                   "clearance fallback uses the final wallplant owner policy");
    return fixture::clearance_result;
}
SurfaceGeometrySample query_motion_wall(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,
        SurfaceGeometryVector,SurfaceGeometryVector,SurfaceGeometryVector,float,SurfaceGeometryTrace*) noexcept {
    fixture::now+=fixture::query_latency;
    return fixture::geometry_mode?fixture::wall:SurfaceGeometrySample{};
}
SurfaceGeometrySample query_wall_geometry(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,
        SurfaceGeometryBody,SurfaceGeometryVector,SurfaceGeometryVector,SurfaceGeometryVector,float,
        SurfaceGeometryTrace*) noexcept{return fixture::geometry_mode?fixture::wall:SurfaceGeometrySample{};}
bool revalidate_surface_geometry(const SurfaceGeometryNativeApi& api,const SurfaceGeometryContext& ctx,
        const SurfaceGeometrySample& sample) noexcept {
    if(!fixture::geometry_mode)return false;
    if(!fixture::clearance_policy)return true;
    // Mirror the production body-owner stamp comparison at this native seam.
    SurfaceGeometryWorldOwner owner{};
    return sample.fresh_for(ctx,fixture::now) && api.static_world_owner &&
        api.static_world_owner(api.user,sample.body,owner) &&
        owner.value && owner.kind==(sample.body_owner?1u:0u) &&
        (!sample.body_owner || owner.value==sample.body_owner) && owner==sample.world_owner;
}
SurfaceGeometryAnchor query_wallplant_anchor(const SurfaceGeometryNativeApi&,const SurfaceGeometryContext&,
        SurfaceGeometryBody,SurfaceGeometryVector point,SurfaceGeometryVector,SurfaceGeometryVector root,SurfaceGeometryVector offset,
        std::span<const SurfaceGeometryVector>,SurfaceGeometryTrace*) noexcept{
    fixture::anchor_point=point;fixture::anchor_offset=offset;
    SurfaceGeometryAnchor result;result.target=root;result.valid=fixture::geometry_mode;return result;
}
}
using namespace dingosdk::arcade_mode;
using namespace dingosdk::arcade_mode::wallplant_runtime;
using fixture::check;
WallplantCommand setup() {
    cancel();fixture::now=1000;fixture::hit_current=true;fixture::race_cancel=false;
    fixture::begins=fixture::releases=fixture::cancels=0;fixture::released_identity=0;
    std::lock_guard lock(mutex);
    interrupt.store(false);stop_pose.store(false);hold_stop={};hold_play={};
    WallplantCommand c{};c.motion.ready=true;c.input_sequence=7;c.contact=3;c.issued_at=1000;
    c.before={-12,-2,5};c.after={6.72f,12.7f,2.8f};c.incoming=c.before;
    c.reflected={12,0,5};c.hold_forward={.92f,0,.38f};c.normal={1,0,0};
    c.contact_normal={.9987492f,.05f,0};c.anchor={1,2,3};c.anchor_verified=true;
    NativeInputState tracker;NativeRequestSample sample{};sample.valid=true;sample.state=201;sample.crouch_mode=2;
    c.native=tracker.observe(990,{1,2,3,4,5},sample,{},false,1.f/30);
    const auto& f=c.native;
    observed=f;permitted=takeoff=true;candidate.valid=true;in_flight=c;
    fixture::active=c;return c;
}
NativeInputFrame ground(const WallplantCommand& c,std::uint64_t at) {
    NativeInputState tracker;NativeRequestSample sample{};sample.valid=true;sample.state=201;sample.crouch_mode=2;
    tracker.observe(c.native.sampled_at,c.native.owner,sample,{},false,1.f/30);
    sample.state=100;sample.crouch_mode=1;sample.words[1]=0x8000;
    for(auto stamp=c.native.sampled_at+33;stamp<at;stamp+=33)
        tracker.observe(stamp,c.native.owner,sample,{},false,1.f/30);
    return tracker.observe(at,c.native.owner,sample,{},false,1.f/30);
}
bool enter(const WallplantCommand& c,std::uint64_t at){fixture::now=at;return begin_hold(c,at);}
bool validate(const WallplantCommand& c,std::uint64_t at,const NativeInputFrame& f){fixture::now=at;return hold_current(c,at,f);}
void observe_at(const NativeInputFrame& f){fixture::now=std::max(fixture::now,f.sampled_at);observe_physics(f);}
void finish_at(const WallplantCommand& c,bool ok,std::uint64_t at){fixture::now=at;finish_hold(c,ok,at);}
bool preserve_at(const NativeInputFrame& f,std::uint64_t at){fixture::now=at;return preserves_combo(f,at);}
void clearance_policy_checks(bool motion) {
    for(unsigned fault=0;fault<7;++fault) {
        setup();in_flight={};candidate={};raw={};pending={};
        const auto f=observed;
        SurfaceGeometryContext ctx{{1,2,3,4,5},context(f),8,7,1,9,f.owner_generation,
            motion?SurfaceGeometryPhase::motion_admission:SurfaceGeometryPhase::collision_response};
        fixture::clearance_policy=fixture::motion_geometry=fixture::geometry_mode=true;
        fixture::clearance_queries=0;fixture::wall={};
        auto& wall=fixture::wall;wall.valid=true;wall.context=ctx;wall.queried_at=1000;
        wall.point={0,2,0};wall.query_normal={1,0,0};wall.body={7,10,1};
        wall.world_owner={{7,70,50331648},77,69,50331648,0,1234};
        auto& fresh_ground=fixture::clearance_result;fresh_ground=wall;
        fresh_ground.body={7,20,1};fresh_ground.body_owner=4321;fresh_ground.point={0,0,0};
        fixture::clearance_owner={{7,80,50331648},88,79,50331648,1,4321};
        fresh_ground.world_owner=fixture::clearance_owner;
        auto ordinary_ground=fresh_ground;ordinary_ground.world_owner={};
        if(fault==1)ordinary_ground=fresh_ground;
        if(fault==2)++fresh_ground.world_owner.generation;
        if(fault==3)++fresh_ground.body_owner;
        if(fault==4)fresh_ground.valid=false;
        if(fault==5)fresh_ground.queried_at=fixture::now+1;
        if(fault==6)fresh_ground.point[1]=1.9f;
        raw.valid=true;raw.owner={1,2,3,4,5};raw.state=201;
        raw.sampled_at=1000;raw.seconds=f.physics_seconds;raw.velocity={-12,-2,0};raw.count=1;
        auto& hit=raw.contacts[0];hit.identity_valid=hit.static_body=hit.categories_known=true;
        hit.normal={1,0,0};hit.point=wall.point;hit.shape={7,10,1};
        raw_frame=f;contact_identity=7;
        SurfaceGeometryNativeApi api;api.now=[](void*)noexcept{return fixture::now;};
        api.current=[](void*,const SurfaceGeometryContext&)noexcept{return true;};
        const std::array<SurfaceGeometryVector,1> supports{{{0,0,0}}};
        const auto token=motion?begin_motion(f,fixture::now):0;
        if(motion)publish_motion_geometry(token,api,ctx,f,{.4f,2,0},{0,1,0},{-12,-2,0},ordinary_ground,supports);
        else publish_geometry(api,ctx,f,{.4f,2,0},ordinary_ground,supports);
        check(candidate.valid==(fault==0 || fault==1),
              "fresh ordinary entity ground requires a current-policy query before wallplant publication");
        check(fixture::clearance_queries==(fault==1?0u:1u),
              "only a fresh ground sample revalidated under current ownership policy avoids the fallback");
        if(candidate.valid) {
            check(candidate.ground.world_owner==fixture::clearance_owner && candidate.ground.body_owner==4321,
                  "published clearance retains the independently queried entity owner stamp");
            check(candidate.ground.queried_at==1000 && candidate.contact.native.sampled_at==f.sampled_at,
                  "clearance reuse correction does not renew native observation or query clocks");
        }
        if(motion)end_motion(token);
        fixture::clearance_policy=fixture::motion_geometry=fixture::geometry_mode=false;
    }
}
void motion_admission_checks() {
    for(unsigned fault=0;fault<11;++fault) {
        setup();in_flight={};candidate={};raw={};pending={};last_used_contact=0;last_handled_input=0;policy.reset();
        auto f=observed;f.airborne_started_known=true;f.airborne_started_at=900;observed=f;
        SurfaceGeometryContext ctx{{1,2,3,4,5},context(f),8,7,1,9,f.owner_generation,SurfaceGeometryPhase::motion_admission};
        fixture::motion_geometry=fixture::geometry_mode=true;fixture::wall={};auto& wall=fixture::wall;
        wall.valid=true;wall.context=ctx;wall.queried_at=1000;wall.point={0,2,0};wall.query_normal={1,0,0};
        wall.body={7,10,1};wall.world_owner={{7,70,50331648},77,69,50331648,0,1234};
        auto ground_sample=wall;ground_sample.point={0,0,0};
        SurfaceGeometryNativeApi api;api.now=[](void*)noexcept{return fixture::now;};
        api.current=[](void*,const SurfaceGeometryContext&)noexcept{return true;};
        const std::array<SurfaceGeometryVector,1> supports{{{0,0,0}}};
        const auto token=begin_motion(f,1000);
        check(token && motion_current(token,f),"motion admission requires an active native invocation");
        check(!begin_motion(f,1000),"nested callbacks cannot replace the admission scope");
        publish_motion_geometry(token,api,ctx,f,{.4f,2,0},{0,1,0},{-12,-2,0},ground_sample,supports);
        check(candidate.valid && candidate.contact.motion_query && !candidate.contact.collision && !raw.serial,
              "motion query is retained without manufacturing a collision");
        check(surface(1000,f).can_plant,"query admits during its originating callback without animation");
        bool foreign_scope=true,foreign_surface=true;
        std::thread other([&]{foreign_scope=motion_current(token,f);foreign_surface=surface(1000,f).can_plant;});other.join();
        check(!foreign_scope && !foreign_surface,"another thread cannot borrow the query or input authority");
        end_motion(token+1);check(motion_current(token,f),"foreign token cannot retire current scope");
        if(fault==1)fixture::now=1035;
        if(fault==2)++f.physics_step;
        if(fault==3)++f.owner_generation;
        if(fault==4)permitted=false;
        if(fault==5)fixture::hit_current=false;
        if(fault==8)end_motion(token);
        if(fault==9)f.state=504;
        InputRequest req{};req.action=InputAction::wall_command;req.variant=1;req.input_sequence=100+fault;
        req.surface_generation=candidate.contact.identity;req.generations={f.owner_generation,f.contact_generation,f.action_generation};
        req.air_episode={f.airborne_episode,f.air_departure};req.physical_pressed_at=1000;
        if(fault==6)req.physical_pressed_at=899;
        if(fault==7)req.physical_pressed_at=1001;
        const auto disposition=request(req,fixture::now,f);
        const auto command=consume(fixture::now,f,{-12,-2,0},req.input_sequence);
        check(bool(command)==(fault==0 || fault==10),"only same-call geometry and fresh airborne A reach the existing command");
        if(fault==10) {
            fixture::now=1035;auto at=fixture::now;
            check(!begin_hold(command,at) && !held_command(),
                  "consumed motion command expires before physical entry without starting a hold");
        }
        if(!fault) {
            check(disposition==InputDisposition::started,"same-call request is admitted");
            check(std::abs(command.after[0]-6.285f)<.0001f && std::abs(command.after[1]-12.7f)<.0001f,
                  "motion path reflects with scalar damping and the reference upward exit");
            bool foreign_current=true,foreign_entry=true;
            std::thread foreign([&]{
                auto at=fixture::now;
                foreign_current=current(command,at,f,command.before);
                foreign_entry=begin_hold(command,at);
            });foreign.join();
            check(!foreign_current && !foreign_entry,"another thread cannot commit a consumed motion command");
            auto at=fixture::now;check(begin_hold(command,at),"same-call command enters the existing hold");
            end_motion(token);
            check(!motion_current(token,f) && held_command() && !latest_completed(),
                  "ending admission preserves an actual hold but cannot claim a release");
            for(std::uint64_t time=1033;time<=1132;time+=33) {
                ++f.physics_step;f.sampled_at=time;fixture::now=time;observe_physics(f);
                check(bool(held_command()),"fresh synthetic AIR observations preserve the planted phase");
            }
            check(!hold.release_due(1159) && hold.release_due(1160),"hold timer separates 160ms plant from release");
        }
        end_motion(token);
        check(!writer_probe().input && !surface(fixture::now,f).can_plant,
              "callback exit discards every unconsumed request and candidate");
    }
    fixture::motion_geometry=fixture::geometry_mode=false;
}
void motion_scope_clock_checks() {
    // Native b661 attempt 17: previous publication 9450, current callback
    // 9479..9489. These are captured clock values; native geometry is mocked.
    setup();in_flight={};candidate={};raw={};pending={};
    auto f=observed;f.sampled_at=1518939450;observed=f;
    fixture::now=1518939479;
    const auto token=begin_motion(f,fixture::now);
    check(token && motion_current(token,f),"captured callback starts with its preceding physics observation");
    fixture::now=1518939489;
    check(motion_current(token,f),
          "ten milliseconds of current callback work cannot expire because the preceding sample is 39ms old");
    check(f.sampled_at==1518939450 && motion_scope.frame.sampled_at==1518939450,
          "accepting the current callback never refreshes its preceding observation");
    fixture::now=1518939513;
    check(motion_current(token,f),"the current callback retains its own 34ms budget at 30Hz");
    fixture::now=1518939514;
    check(!motion_current(token,f),"a callback exceeding its own physics-step budget is refused");
    fixture::now=1518939478;
    check(!motion_current(token,f),"a clock preceding callback entry is refused");
    fixture::now=1518939489;
    auto changed=f;++changed.owner_generation;
    check(!motion_current(token,changed),"captured timing cannot authorize another owner generation");
    changed=f;++changed.physics_step;
    check(!motion_current(token,changed),"captured timing cannot authorize another physics observation");
    changed=f;changed.state=504;
    check(!motion_current(token,changed),"captured timing cannot authorize an offboard frame");
    check(!motion_current(token+1,f),"captured timing cannot authorize a foreign token");
    bool foreign=true;
    std::thread other([&]{foreign=motion_current(token,f);});other.join();
    check(!foreign,"captured timing cannot authorize another thread");
    permitted=false;check(!motion_current(token,f),"Arcade OFF withdraws the current callback");
    permitted=true;++observed.physics_step;
    check(!motion_current(token,f),"a newly published observation invalidates the old callback");
    observed=f;end_motion(token);
    check(!motion_current(token,f),"callback return withdraws admission immediately");

    // The observation lease and callback budget are independent. Neither is
    // renewed by scope entry, even if the other still has time remaining.
    fixture::now=f.sampled_at+140;
    const auto late=begin_motion(f,fixture::now);
    check(late!=0,"an observation inside its established lease can start a scope");
    fixture::now=f.sampled_at+InputTiming::native_sample_lease_ms;
    check(motion_current(late,f),"observation lease remains inclusive at its existing boundary");
    ++fixture::now;
    check(!motion_current(late,f),"a stale observation is refused even within the current callback budget");
    end_motion(late);
    check(!begin_motion(f,fixture::now),"scope entry cannot renew an expired observation");
    fixture::now=f.sampled_at-1;
    check(!begin_motion(f,fixture::now),"a future observation cannot start a scope");
}

void motion_geometry_diagnostic_checks() {
    for(unsigned fault=0;fault<5;++fault) {
        setup();in_flight={};candidate={};raw={};pending={};
        const auto f=observed;
        SurfaceGeometryContext ctx{{1,2,3,4,5},context(f),8,7,1,9,f.owner_generation,SurfaceGeometryPhase::motion_admission};
        fixture::motion_geometry=fixture::geometry_mode=true;fixture::wall={};
        auto& wall=fixture::wall;wall.valid=true;wall.context=ctx;wall.queried_at=1000;
        wall.point={0,2,0};wall.query_normal={1,0,0};
        wall.body={7,10,1};wall.world_owner={{7,70,50331648},77,69,50331648,0,1234};
        auto ground_sample=wall;ground_sample.point={0,0,0};
        SurfaceGeometryNativeApi api;api.now=[](void*)noexcept{return fixture::now;};
        api.current=[](void*,const SurfaceGeometryContext&)noexcept{return true;};
        const std::array<SurfaceGeometryVector,1> supports{{{0,0,0}}};
        auto span=std::span<const SurfaceGeometryVector>(supports);
        if(fault==1)span={};
        if(fault==2)wall.valid=false;
        fixture::query_latency=fault==3?35:(fault==4?25:0);
        const auto token=begin_motion(f,1000);
        MotionDiagnostic diagnostic{};
        publish_motion_geometry(token,api,ctx,f,{.4f,2,0},{0,1,0},{-12,-2,0},ground_sample,span,&diagnostic);
        const char* expected[]{"published","motion_geometry_guard","motion_wall","motion_publication_guard","published"};
        check(std::string_view(diagnostic.geometry_reason)==expected[fault],
              "geometry receipt distinguishes failed geometry, expired callback and valid work after the prior frame interval");
        check(candidate.valid==(fault==0 || fault==4),
              "diagnostic output cannot turn failed geometry or expired scope into an admissible candidate");
        check(f.sampled_at==990 && motion_scope.frame.sampled_at==990,
              "diagnostics preserve the original sample clock through a delayed query");
        end_motion(token);fixture::query_latency=0;
    }
    fixture::motion_geometry=fixture::geometry_mode=false;
}
void motion_diagnostic_checks() {
    setup();in_flight={};candidate={};pending={};
    auto f=observed;const auto token=begin_motion(f,fixture::now);
    check(token!=0,"diagnostic fixture starts a real runtime scope");
    MotionDiagnostic row{};row.native=f;row.token=token;row.started=fixture::now;
    row.reason="input_claim";row.geometry_reason="motion_wall";row.trace.reason="no_hit";
    row.request_word=0x408000;row.trace.ray=2;
    end_motion(token);
    const auto before=fixture::log_calls;
    motion_diagnostic(row);
    check(fixture::log_calls==before && motion_count==1 && motion_read==0,
          "receipt enqueue never formats or emits logging on the physics callback");
    row.request_word=0;row.trace.ray=4;
    check(motion_rows[0].request_word==0x408000 && motion_rows[0].trace.ray==2 &&
          motion_rows[0].native.sampled_at==f.sampled_at,
          "queued evidence is an immutable copy and never refreshes the frame");
    check(!candidate.valid && !writer_probe().input && !held_command() && !motion_current(token,f),
          "an admission receipt cannot create geometry, input, scope or hold authority");
    flush_motion_diagnostics();
    check(motion_read==1 && fixture::log_calls>before && !motion_queued.load(),
          "deferred flush emits and drains the real receipt");
    const auto after=fixture::log_calls;flush_motion_diagnostics();
    check(fixture::log_calls==after,"drained observations are not emitted twice");
    for(unsigned i=0;i<130;++i)motion_diagnostic(row);
    check(motion_count==128 && motion_dropped==3,"diagnostic storage has a fixed process bound and counts overflow");
    flush_motion_diagnostics();
    check(motion_read==128 && motion_reported_dropped==3 && !motion_queued.load(),
          "overflow is reported rather than silently interpreted as absence of attempts");
    const auto full=fixture::log_calls;motion_diagnostic(row);flush_motion_diagnostics();
    check(motion_dropped==4 && fixture::log_calls==full && !motion_queued.load(),
          "process capacity also bounds log output after the single overflow notice");
}
int main(){
    clearance_policy_checks(true);
    clearance_policy_checks(false);
    motion_diagnostic_checks();
    motion_admission_checks();
    motion_scope_clock_checks();
    motion_geometry_diagnostic_checks();
    {
        const auto c=setup();check(enter(c,1000),"pose interruption fixture enters");
        interrupt_animation();fixture::now=1030;tick(1,true);
        check(bool(held_command()) && fixture::begins==0 && hold.started_at()==1000,
              "pose interruption neither starts a queued pose nor cancels or renews the physical plant");
        const auto released=ground(c,1160);check(validate(c,1160,released),"release continuity remains valid after pose interruption");
        finish_at(c,true,1160);interrupt_animation();fixture::now=1161;tick(1,true);
        check(latest_completed().input_sequence==c.input_sequence && preserve_at(released,1161),
              "air trick pose interruption preserves the existing release and combo bridge");
        cancel();check(!held_command() && !preserve_at(released,1161),"Arcade OFF still withdraws physical and combo ownership");
    }
    for(unsigned fault=0;fault<13;++fault) {
        const auto command=setup();const auto origin=command.native;
        in_flight={};candidate={};contact_active=false;last_raw=0;
        auto& raw_contact=fixture::collision;raw_contact={};
        raw_contact.valid=raw_contact.after_valid=true;raw_contact.serial=1;raw_contact.sampled_at=1000;
        raw_contact.owner={1,2,3,4,5};raw_contact.state=raw_contact.state_after=201;
        raw_contact.source=native_wall_contact::Source::board;raw_contact.seconds=origin.physics_seconds;
        raw_contact.count=1;auto& hit=raw_contact.contacts[0];
        hit.identity_valid=hit.static_body=hit.categories_known=true;hit.normal={1,0,0};
        fixture::collision_scope=true;
        auto frame=origin;
        if(fault==1)fixture::collision_scope=false;
        if(fault==2)++frame.owner_generation;
        if(fault==3)takeoff=false;
        if(fault==4)raw_contact.state_after=504;
        if(fault==5)raw_contact.seconds=0;
        if(fault==6)raw_contact.sampled_at=989;
        if(fault==7)raw_contact.sampled_at=1001;
        if(fault==8)hit.static_body=false;
        if(fault==9)hit.normal={0,1,0};
        if(fault==10)raw_contact.source=native_wall_contact::Source::rig;
        if(fault==11)permitted=false;
        if(fault==12)last_raw=raw_contact.serial;
        check(!retain_collision_before_response(frame,1000),
              "late collision callbacks cannot admit the motion-boundary alternative");
        check(wallplant_same_observation(observed,origin),
              "early contact retention never renews a native frame or manufactures a physics step");
        check(!surface(1000,origin).can_plant && !pending.sequence && !held_command(),
              "early raw contact alone cannot bypass geometry, fresh A or hold entry");
    }
    fixture::collision={};fixture::collision_scope=false;
    {
        const auto c=setup();const auto& f=c.native;
        SurfaceGeometryContext ctx{{1,2,3,4,5},context(f),8,7,1,9,f.owner_generation,SurfaceGeometryPhase::collision_response};
        fixture::geometry_mode=true;fixture::wall={};fixture::wall.context=ctx;
        fixture::wall.valid=true;fixture::wall.queried_at=1000;
        fixture::wall.point={.02f,1.5f,.03f};fixture::wall.query_normal={1,0,0};
        SurfaceGeometrySample ground_sample=fixture::wall;ground_sample.point={0,0,0};
        raw={};raw.valid=true;raw.owner={1,2,3,4,5};raw.state=201;
        raw.sampled_at=1000;raw.seconds=f.physics_seconds;raw.velocity={-10,0,0};raw.count=1;
        auto& hit=raw.contacts[0];hit.identity_valid=hit.static_body=hit.categories_known=true;
        hit.normal={1,0,0};hit.point={0,1.5f,0};hit.shape={7,1,0};
        raw_frame=f;contact_identity=7;
        SurfaceGeometryNativeApi api;api.now=[](void*)noexcept{return fixture::now;};
        api.current=[](void*,const SurfaceGeometryContext&)noexcept{return true;};
        const std::array<SurfaceGeometryVector,1> support{{{0,-.8f,-.6f}}};
        publish_geometry(api,ctx,f,{.2f,2.3f,0},ground_sample,support);
        check(candidate.valid,"guarded collision geometry reaches real publication");
        check(fixture::anchor_point==fixture::wall.point&&fixture::anchor_offset==support[0],
              "anchor uses measured wall plane and native board origin rather than raw contact and entity origin");
        check(candidate.contact.point==fixture::wall.point&&candidate.hit.point==hit.point,
              "visual point uses corroborated wall while original collision observation is retained");
        fixture::geometry_mode=false;
    }
    for(bool missing:{false,true}) {
        auto c=setup();if(missing)c.anchor_verified=false;else ++c.anchor[0];
        check(!enter(c,1000) && !held_command(),"changed or unverified entry anchor cannot borrow consumed geometry authority");
    }
    {
        auto c=setup();check(enter(c,1000),"successful entry starts physical hold");
        check(!latest_completed(),"entry is not a completed rebound");
        check(!enter(c,1001),"entry cannot duplicate active phase");
        const auto f=ground(c,1033);observe_at(f);fixture::now=1035;tick(1,true);
        check(fixture::begins==1 && fixture::visual_normal==c.contact_normal,"visual begins during contact100 using actual wall plane");
        check(fixture::visual_expiry==1194,"visual shares the original physical dispatch expiry without renewing its clock");
        fixture::now=1040;tick(1,true);check(fixture::begins==1,"same hold does not queue another pose");
        finish_at(c,false,1040);fixture::now=1041;tick(1,true);
        check(fixture::releases==1 && fixture::released_identity==c.input_sequence,"cancellation retains stop identity for client");
        check(!latest_completed(),"cancelled hold creates no rebound occurrence");
    }
    {
        auto c=setup();check(enter(c,1000),"release test enters");
        observe_at(ground(c,1033));
        auto f=ground(c,1165);observe_at(f);fixture::now=1175;
        check(validate(c,1175,f),"release revalidates original hold with current native sample");
        finish_at(c,true,1175);
        check(latest_completed().input_sequence==7 && !held_command(),"actual release publishes once and removes motion authority");
        check(preserve_at(f,1200),"existing contact100 has one native interval from actual release");
        auto expired=combo_bridge;
        check(!expired.observe(f,1210),"release bridge does not grant a second native interval");
        check(!combo_bridge.target_active(1180) && !combo_bridge.release_due(1180),"combo bridge grants no target or release rights");
        auto air=f;air.state=200;air.airborne=true;++air.airborne_episode;++air.physics_step;air.sampled_at=1205;
        ++air.action_generation;++air.contact_generation;air.mode=2;air.contacts=0;
        observe_at(air);check(!combo_bridge.active(),"first genuine resumedAIR retires bridge");
        auto landed=air;landed.state=100;landed.airborne=false;++landed.air_departure;++landed.physics_step;landed.sampled_at=1208;
        check(!preserve_at(landed,1208),"real landing after resumedAIR cannot inherit suppression");
        finish_at(c,true,1208);check(latest_completed().issued_at==1000,"duplicate finish cannot renew completed occurrence");
    }
    {
        auto c=setup();check(enter(c,1000),"air release test enters");
        auto air=c.native;air.sampled_at=1050;++air.physics_step;observe_at(air);
        air.sampled_at=1160;++air.physics_step;observe_at(air);
        finish_at(c,true,1160);check(!combo_bridge.active(),"release already inAIR cannot suppress a later real ground contact");
    }
    for(unsigned fault=0;fault<14;++fault){
        auto c=setup();check(enter(c,1000),"immutable identity fixture enters");auto changed=c;
        switch(fault){
        case 0:++changed.native.owner_generation;break;case 1:++changed.native.physics_step;break;
        case 2:++changed.native.sampled_at;break;case 3:++changed.contact;break;
        case 4:++changed.geometry.world;break;case 5:++changed.body.generation;break;
        case 6:++changed.point[0];break;case 7:++changed.contact_normal[1];break;
        case 8:++changed.before[0];break;case 9:++changed.after[0];break;
        case 10:++changed.incoming[0];break;case 11:++changed.reflected[0];break;
        case 12:++changed.hold_forward[0];break;case 13:++changed.anchor[0];break;
        }
        check(!validate(changed,1033,ground(c,1033)) && bool(held_command()),"changed command cannot borrow or cancel legitimate held authority");
        finish_at(changed,true,1160);check(!latest_completed() && bool(held_command()),"changed completion cannot manufacture release");
    }
    for(unsigned fault=0;fault<3;++fault){
        auto c=setup();check(enter(c,1000),"invalid continuity fixture enters");auto f=ground(c,1033);
        if(fault==0)++f.owner_generation;
        if(fault==1)f.state=504;
        if(fault==2)fixture::hit_current=false;
        check(!validate(c,1033,f) && !held_command(),"invalid live ownership/state/contact cancels hold");
        check(hold_stop.input_sequence==7,"destructive continuity rejection preserves original visual stop receipt");
        fixture::now=1034;tick(1,true);
        check(fixture::released_identity==7 && fixture::begins==0,"cancelled queued visual never starts late");
    }
    {
        auto c=setup();check(enter(c,1000),"physics cancellation fixture enters");
        auto f=ground(c,1033);f.state=504;observe_at(f);
        check(!held_command() && hold_stop.input_sequence==7,"observer rejection preserves stop before helper clears command");
    }
    {
        auto c=setup();check(enter(c,1000),"request race fixture enters");
        fixture::race_cancel=true;fixture::now=1030;tick(1,true);
        check(fixture::begins==1 && fixture::cancels==1 && !held_command(),"release during visual request cancels accepted pose immediately after request");
    }
    {
        auto c=setup();check(enter(c,1000),"delayed publication fixture enters");
        const auto f=ground(c,1033);std::uint64_t stale_now=1033;fixture::now=1040;
        check(hold_current(c,stale_now,f) && stale_now==1040,"target validation returns clock refreshed inside runtime lock");
        fixture::now=1045;observe_physics(f);
        check(bool(held_command()) && observed.sampled_at==1033,"delayed native publication does not backdate validation or renew observation");
        auto late=ground(c,1165);fixture::now=1180;stale_now=1165;
        check(hold_current(c,stale_now,late) && stale_now==1180,"late callback validates actual current clock");
        fixture::now=1181;finish_hold(c,true,1165);
        check(latest_completed().input_sequence==7,"completion samples current clock rather than stale caller time");
    }
    {
        auto c=setup();check(enter(c,1000),"late dispatch fixture enters");
        check(!validate(c,1195,ground(c,1195)),"expired physical phase cannot release late");
        finish_at(c,true,1195);check(!latest_completed(),"expired release cannot publish success");
    }
    {
        auto c=setup();check(enter(c,1000),"interleaved reader fixture enters");
        const auto reader_frame=ground(c,1033);observe_at(reader_frame);
        const auto newer=ground(c,1066);observe_at(newer);
        fixture::now=1070;std::uint64_t at=1033;bool stale=false;
        const bool authorized=hold_current(c,at,reader_frame,&stale);
        check(bool(held_command()) && hold.started_at()==1000 && hold.deadline()==1160,
            "stale query preserves newer hold without extending its deadline");
        check(!authorized && stale,"reader overtaken by original physics cannot authorize a target");
        check(hold_current(c,at,newer,&stale) && !stale,
            "fresh reader can use the independently published newer frame");
        fixture::now=1195;
        check(!hold_current(c,at,reader_frame,&stale) && !stale && !held_command(),
            "expired hold cancels even when reader is stale");
    }
    for(unsigned fault=0;fault<3;++fault) {
        auto c=setup();check(enter(c,1000),"stale query safety fixture enters");
        auto older=ground(c,1033);observe_at(ground(c,1066));
        if(fault==0)++older.owner_generation;
        if(fault==1)older.state=504;
        if(fault==2)fixture::hit_current=false;
        fixture::now=1070;std::uint64_t at=1070;bool stale=true;
        check(!hold_current(c,at,older,&stale) && !stale && !held_command(),
            "owner, bail and collision loss cannot hide behind stale-query deferral");
    }
    std::cout<<fixture::checks<<" actual runtime hold lifecycle checks passed\n";
}
