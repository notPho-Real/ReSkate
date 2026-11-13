#include "Engine/Game/World/body_drive_lease.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
using namespace dingosdk::body_drive;
namespace {
void require(bool ok, const char* reason) { if (!ok) { std::cerr << reason << '\n'; std::exit(1); } }
constexpr World world{1,2,3};
constexpr NativeHandle body{2,7,11};
constexpr Pose pose{0,0,0,1,2,3,4,1};
struct Backend {
    World live=world;
    std::vector<std::string> calls;
    bool fail_anchor{}, fail_joint{}, fail_target{}, fail_remove{}, known=true;
    bool current(World w) const { return w==live; }
    bool cleanup_known() const { return known; }
    bool valid_pose(const Pose& p) const { return std::all_of(p.begin(),p.end(),[](float v){return std::isfinite(v);}); }
    bool body_valid(NativeHandle h) const { return h.world==live.world && (h==body || h.index==20); }
    bool joint_valid(NativeHandle h) const { return h.world==live.world && h.index==30; }
    bool create_anchor(World w,const Pose&,NativeHandle& h) { calls.push_back("anchor");h={w.world,20,21};return !fail_anchor; }
    bool create_joint(World w,NativeHandle b,NativeHandle a,NativeHandle& h) {
        require(b==body && a.index==20,"wrong attachment");calls.push_back("joint");h={w.world,30,31};return !fail_joint;
    }
    bool enable(NativeHandle h) { require(h.index==30,"enabled borrowed body");calls.push_back("enable");return true; }
    bool target(NativeHandle h,const Pose&) { require(h.index==20,"moved borrowed body");calls.push_back("target");return !fail_target; }
    bool remove_joint(World,NativeHandle& h) { require(h.index==30,"released foreign joint");calls.push_back("remove_joint");if(fail_remove)return false;h={};return true; }
    bool remove_anchor(World,NativeHandle& h) { require(h.index==20,"released borrowed body");calls.push_back("remove_anchor");h={};return true; }
};
}
void run_native_drive_cleanup_tests();
int main() {
    run_native_drive_cleanup_tests();
    {
        Backend b;Lease lease;
        require(lease.begin(b,world,body,pose),"begin failed");
        require(!lease.begin(b,world,body,pose),"duplicate begin allocated");
        require(lease.target(b,world,body,pose),"target failed");
        const auto n=b.calls.size();
        auto replacement=body;++replacement.generation;
        require(!lease.target(b,world,replacement,pose) && b.calls.size()==n,"recycled body accepted");
        require(lease.end(b,world),"release failed");
        require(!lease.active() && !lease.occupied(),"release retained authority");
        require(b.calls==std::vector<std::string>{"anchor","joint","enable","target","remove_joint","remove_anchor"},"lifecycle order wrong");
        const auto ended=b.calls.size();
        require(lease.end(b,world) && b.calls.size()==ended,"release repeated native writes");
    }
    for (bool fail_joint : {false,true}) {
        Backend b;Lease lease;b.fail_anchor=!fail_joint;b.fail_joint=fail_joint;
        require(!lease.begin(b,world,body,pose),"partial allocation succeeded");
        require(!lease.occupied(),"known partial allocation leaked");
        require(b.calls.back()=="remove_anchor","anchor freed before joint cleanup");
        if(fail_joint) require(b.calls[b.calls.size()-2]=="remove_joint","partial joint leaked");
    }
    {
        Backend b;Lease lease;b.fail_joint=true;b.known=false;
        require(!lease.begin(b,world,body,pose) && lease.occupied() && lease.uncertain(),"unknown allocation forgotten");
        const auto n=b.calls.size();
        require(!lease.begin(b,world,body,pose) && !lease.end(b,world) && b.calls.size()==n,"uncertain acquisition retried or freed");
    }
    {
        Backend b;Lease lease;require(lease.begin(b,world,body,pose),"begin failed");
        b.fail_target=true;require(!lease.target(b,world,body,pose),"failed target accepted");
        const auto n=b.calls.size();
        require(!lease.target(b,world,body,pose) && b.calls.size()==n,"uncertain target retried");
        require(lease.end(b,world),"target failure prevented ordered cleanup");
    }
    {
        Backend b;Lease lease;require(lease.begin(b,world,body,pose),"begin failed");
        b.fail_remove=true;require(!lease.end(b,world),"failed release accepted");
        const auto n=b.calls.size();
        require(lease.occupied() && lease.uncertain() && !lease.end(b,world) && b.calls.size()==n,"uncertain removal retried");
        require(std::find(b.calls.begin(),b.calls.end(),"remove_anchor")==b.calls.end(),"anchor freed under live joint");
    }
    {
        Backend b;Lease lease;require(lease.begin(b,world,body,pose),"begin failed");
        b.live={4,5,6};const auto n=b.calls.size();
        require(!lease.end(b,b.live) && !lease.target(b,b.live,body,pose) && b.calls.size()==n,"stale world touched");
        require(!lease.world_destroyed(b.live) && lease.occupied(),"unrelated teardown discarded lease");
        require(lease.world_destroyed(world) && !lease.occupied(),"confirmed teardown retained stale handle");
    }
    std::cout << "temporary drive ownership regressions passed\n";
}
