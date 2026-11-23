#include "Extension/Trainer/trainer_trajectory_update.h"
#include <cstdio>
#include <cstdlib>
#include <limits>

using namespace dingosdk::trainer;
namespace native=dingosdk::game::build::v20260929::trajectory;
namespace {
unsigned checks{};
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
// Bounded replay of the pinned selector's notification contract, not a native
// physics emulator. 4755ce8 announces a first result only for updates/requeries;
// 4822292 consumes it before 48256b0 samples the new curve with its frame clock.
struct Query {
    bool pending{},completed{},air_update{},changed{},selected{},owner{true};
    unsigned submissions{},selections{},rebases{},frame{27};
    bool settings_valid{true};
    float first{.125f},second{2.5f};
    NativeTrajectoryParameters packet{};
    bool submit(std::uintptr_t entry,float* values) {
        if(pending)return false;
        check(entry==native::update_air.rva || entry==native::set.rva,"only known native entries");
        air_update=entry==native::update_air.rva;pending=true;selected=false;++submissions;
        std::copy_n(values,packet.values.size(),packet.values.begin());return true;
    }
    void select() {
        ++selections;changed=false;
        if(!pending || !completed)return;
        changed=air_update;pending=false;selected=true;
    }
    void tick() {
        select();if(changed){frame=0;++rebases;}++frame;
    }
    bool update(const NativeTrajectoryParameters& values) {
        return trajectory_detail::update_air(values,[&](float& a,float& b){a=first;b=second;return settings_valid;},[&]{return owner && !pending;},
            [&](std::uintptr_t entry,float* packet_values){return submit(entry,packet_values);});
    }
};
NativeTrajectoryParameters recorded() {
    NativeTrajectoryParameters p{};
    // contact-chain-hil/vert_level_out_7feb.json, physical step48615.
    const float velocity[]{-.00091552734375f,.8800638914108276f,-2.773805618286133f};
    std::copy_n(velocity,3,p.values.begin()+native::parameter_velocity/sizeof(float));
    // Synthetic distinct settings expose accidental reuse; only velocity above is captured.
    p.values[native::parameter_setting_first/sizeof(float)]=19.f;
    p.values[native::parameter_setting_second/sizeof(float)]=37.f;
    p.values[native::parameter_reference_velocity/sizeof(float)+1]=-2.96978235f;
    reinterpret_cast<unsigned char*>(p.values.data())[native::parameter_launch]=1;
    return p;
}
void handoff() {
    auto p=recorded();Query old;
    check(old.submit(native::set.rva,p.values.data()),"old midflight launch was submitted");
    old.completed=true;old.tick();
    check(old.selected && !old.rebases && old.frame==28,"old first result keeps prior curve clock");
    Query current;
    check(current.update(p),"owned flight submits the native AIR update");
    check(current.submissions==1 && current.selections==0,"adapter never selects or retries");
    current.tick();
    check(current.pending && !current.rebases,"unfinished query is not a replacement or landing");
    current.completed=true;current.tick();
    check(current.selected && current.rebases==1 && current.frame==1,"first completed AIR update rebases before sampling");
    current.tick();check(current.rebases==1 && current.frame==2,"native clock continues without repeated reset");
    // The measured new curve would otherwise be sampled at the previous
    // flight's elapsed time. This demonstrates the discontinuity class only;
    // articulated native velocities/orientation still require runtime proof.
    const auto height=[](unsigned frame){const float t=float(frame)/60.f;return 6.04264688f+1.16896129f*t-4.9f*t*t;};
    check(std::abs(height(old.frame)-height(1))>.5f,"recorded replacement curve exposes stale clock position jump");
    Query eager;eager.completed=true;check(eager.update(p),"completed synchronous update submits");
    check(eager.selections==0,"even a ready result is reserved for native selection");
    eager.select();check(eager.changed,"first native selector publishes change");
    eager.tick();check(!eager.rebases,"extra eager selection loses notification before native AIR tick");
}
void packets_and_refusal() {
    const auto p=recorded();Query q;check(q.update(p),"recorded finite packet accepted");
    const auto* bytes=reinterpret_cast<const unsigned char*>(q.packet.values.data());
    check(q.packet.values[native::parameter_setting_first/sizeof(float)]==q.first &&
        q.packet.values[native::parameter_setting_second/sizeof(float)]==q.second,
        "AIR uses its native tuning pair instead of the generic launch pair");
    check(p.values[native::parameter_setting_first/sizeof(float)]==19.f &&
        p.values[native::parameter_setting_second/sizeof(float)]==37.f,"caller settings unchanged");
    Query missing;missing.settings_valid=false;
    check(!missing.update(p) && !missing.submissions,"unavailable tuning asset refuses native update");
    Query invalid_settings;invalid_settings.first=std::numeric_limits<float>::quiet_NaN();
    check(!invalid_settings.update(p) && !invalid_settings.submissions,"nonfinite first setting refused");
    invalid_settings.first=1.f;invalid_settings.second=std::numeric_limits<float>::infinity();
    check(!invalid_settings.update(p) && !invalid_settings.submissions,"nonfinite second setting refused");
    check(bytes[native::parameter_launch]==0,"AIR does not request slope/pop shaping");
    for(unsigned i=0;i<3;++i)check(q.packet.values[native::parameter_velocity/sizeof(float)+i]==
        q.packet.values[native::parameter_reference_velocity/sizeof(float)+i],"world velocity pair is coherent");
    check(reinterpret_cast<const unsigned char*>(p.values.data())[native::parameter_launch]==1,"caller packet unchanged");
    check(!q.update(p) && q.submissions==1 && q.selections==0,"busy query fails without consumption or second action");
    Query lost;lost.owner=false;check(!lost.update(p) && !lost.submissions,"lost caller owner permission refuses submission");
    for(unsigned index:{0u,unsigned(native::parameter_velocity/sizeof(float)),unsigned(native::parameter_reference_velocity/sizeof(float))}) {
        auto invalid=p;invalid.values[index]=std::numeric_limits<float>::quiet_NaN();Query bad;
        check(!bad.update(invalid) && !bad.submissions,"invalid transform/velocity packet cannot enter native code");
    }
    auto huge=p;huge.values[native::parameter_velocity/sizeof(float)]=1000;Query bad;
    check(!bad.update(huge) && !bad.submissions,"unbounded requested velocity refused");
}
}
int main(){handoff();packets_and_refusal();std::printf("%u native trajectory contract checks passed; native gameplay unverified\n",checks);}
