#include "Extension/ArcadeMode/arcade_native_input_state.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <latch>
#include <thread>
namespace {std::atomic<std::uint64_t> tick{};unsigned checks{};}
namespace dingosdk::arcade_mode {std::uint64_t balance_hud_test_now() noexcept{return tick.load();}}
#define DINGOSDK_BALANCE_HUD_TEST_CLOCK
#include "../arcade_balance_hud.cpp"
using namespace dingosdk::arcade_mode;
namespace {
void check(bool v,const char* message){++checks;if(!v){std::cerr<<message<<'\n';std::exit(1);}}
void publish(const NativeInputFrame& native_frame){State display;display.combo_active=true;display.balance_kind=BalanceKind::manual;display.balance=.25f;
 // Exact mode payload: no age reconstruction or client/render restamping.
 publish_balance_hud(display,0,BalanceHudSource{native_frame.sampled_at,native_frame.valid});
}
}
int main(){
 for(bool nose:{false,true})for(std::uint64_t base:{0ull,1000ull,0xfffffff0ull}){
  NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};NativeRequestSample sample{};
  sample.valid=true;sample.state=100;sample.crouch_mode=1;sample.words[1]=nose?0x8000u:0x4000u;
  tick=base;const auto frame=producer.observe(tick,owner,sample,{true,-1});
  check(frame.valid&&frame.sampled_at==base,"actual native producer keeps tick-domain origin including zero and32-bit wrap");
  tick=base+149;publish(frame);check(balance_hud_pending(),"same-domain physical source is live at149ms");
  tick=base+150;publish(frame);check(balance_hud_pending(),"unchanged-source republication keeps inclusive150ms endpoint");
  tick=base+151;publish(frame);check(!balance_hud_pending(),"unchanged-source republication cannot borrow a new expiry");
  tick=base+200;publish(frame);check(!balance_hud_pending(),"cached native frame cannot revive expired presentation");
  tick=base+210;const auto newer=producer.observe(tick,owner,sample,{true,-1});publish(newer);check(balance_hud_pending(),"genuine new original physics source rearms presentation");
  tick=base+359;
  std::unique_lock lock(hud_mutex);std::latch called(1);
  std::thread worker([&]{called.count_down();publish(newer);});called.wait();tick=base+361;lock.unlock();worker.join();
  check(!balance_hud_pending(),"source149ms plus2ms real publisher-mutex delay cannot renew original expiry");
  publish_balance_hud({},0,{});check(!balance_hud_pending(),"mode clear has explicit invalid source");
 }
 std::cout<<checks<<" actual native producer to HUD expiry checks passed\n";
}
