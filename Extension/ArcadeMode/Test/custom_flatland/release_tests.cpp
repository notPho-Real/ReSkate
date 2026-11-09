#include "Extension/ArcadeMode/arcade_custom_flatland_runtime.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
int main(){
    // Link without animation endpoints: a release must have no live producer.
    NativeInputState observer;
    NativeRequestSample sample{};sample.valid=true;sample.state=100;sample.crouch_mode=1;sample.words[1]=0x4000u;
    const auto frame=observer.observe(1000,{1,2,3,4,5},sample,{true,-1},false,1.f/60);
    CustomFlatlandContext context{frame,1000,1,true,true,true,false,false};
    if(!context.valid())return EXIT_FAILURE;
    custom_flatland_runtime::set_enabled(true);
    if(custom_flatland_runtime::status().enabled)return EXIT_FAILURE;
    custom_flatland_runtime::tick(context);
    const auto binding=custom_flatland_runtime::observe(context);
    InputRequest request{};request.context=InputContext::balance;
    request.action=InputAction::balance_branch_one;request.variant=2;
    request.generations=InputNativeContext::from_frame(frame).generations;
    const auto result=custom_flatland_runtime::request(request);
    const auto motion=custom_flatland_runtime::consume(frame,1000,{3,0,4},true);
    custom_flatland_runtime::cancel();
    if(binding.enabled || binding.branch || binding.ready || result!=InputDisposition::unsupported ||
        motion.admitted || motion.x!=0 || motion.z!=0)return EXIT_FAILURE;
    std::cout<<"Release custom flatland bridge remains disabled without animation dependencies\n";
}
