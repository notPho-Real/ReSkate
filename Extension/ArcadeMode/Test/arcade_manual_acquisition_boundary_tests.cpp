#include "Extension/ArcadeMode/arcade_manual_acquisition.h"
#include "Extension/ArcadeMode/arcade_manual_release.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool value,const char* text){++checks;if(!value){std::cerr<<text<<'\n';std::exit(1);}}
struct Pipeline {
 bool nose{};NativeInputState producer;NativeInputOwner owner{1,2,3,4,5};NativeInputFrame frame{};
 ManualAcquisition acquisition;ManualRelease release;ManualInputDiagnostic command{};
 explicit Pipeline(bool n):nose(n){command.owns=true;command.request=n?1:-1;command.request_at=1000;}
 bool step(std::uint64_t now,unsigned state=100,bool admitted=false,bool eligible=true,bool push=false){
  NativeRequestSample sample{};sample.valid=true;sample.state=state;sample.crouch_mode=1;sample.words[1]=state==100?(nose?0x8000u:0x4000u):0;
  frame=producer.observe(now,owner,sample,{true,-1});
  // Mode's actual order: cleanup joins the physical observation; remap later
  // publishes command delivery. Both consume the same original native frame.
  const auto physical=manual_acquisition_pose(frame);
  const bool exit=acquisition.step(now,frame,physical,admitted,eligible,push);
  const bool context=state==100||state==103||state==200||state==201||frame.rail_family;
  const auto output=release.step(now,frame.owner_generation,eligible&&context,physical,exit);
  acquisition.observe(now,frame,command,eligible&&!push);
  return output.pending;
 }
 void acquire(){step(1000);step(1017);for(std::uint64_t now=1100;now<=2100;now+=100)check(!step(now),"unadmitted acquired request retains its original deadline before boundary");}
};
}
int main(){
 for(bool nose:{false,true})for(unsigned boundary:{401u,402u,403u,404u,499u,701u})for(bool observed:{false,true}){
  Pipeline p(nose);p.acquire();p.command.owns=false;
  if(observed){
   check(!p.step(2190,boundary),"known rail/exit transition cannot issue Manual putdown");
   check(!p.step(2207,701),"rail exit cannot retain or rearm withdrawn acquisition");
  }else{
   // Exercise remap observe independently of a client cleanup sample.
   NativeRequestSample s{};s.valid=true;s.state=boundary;s.crouch_mode=1;
   auto f=p.producer.observe(2190,p.owner,s,{true,-1});p.acquisition.observe(2190,f,p.command,true);
  }
  check(!p.step(2224),"later unrelated matching Manual contact must not inherit old2200 cleanup deadline");
  p.command.owns=true;p.command.request_at=2241;p.step(2241);p.step(2258);
  for(std::uint64_t now=2300;now<3400;now+=100)check(!p.step(now),"new genuine request owns its own original deadline");
  check(!p.step(3440),"new request remains below original1199ms boundary");
  check(p.step(3441),"new request expires at its own original1200ms boundary");
 }
 for(bool nose:{false,true})for(unsigned boundary:{401u,402u,403u,404u,499u,701u}){
  Pipeline p(nose);p.acquire();check(p.step(2200),"setup has an actually pending old putdown");p.command.owns=false;
  check(!p.step(2217,boundary),"positive rail/exit evidence also withdraws an already pending release owner");
  check(!p.step(2234),"completed old release cannot affect unrelated later contact");
 }
 for(bool nose:{false,true}){
  Pipeline p(nose);p.acquire();p.command.owns=false;p.step(2190);
  p.acquisition.observe(2190,p.frame,p.command,false);
  check(!p.step(2224),"authored/action ownership cancellation in remap retires the old acquired cleanup token");
 }
 for(bool nose:{false,true}){
  Pipeline p(nose);p.acquire();p.command.owns=false;
  check(!p.step(2199,101),"unknown native settling does not fabricate a completion at1199ms");
  check(p.step(2200),"unknown settling preserves bounded acquired cleanup at1200ms");
  Pipeline admitted(nose);admitted.acquire();check(!admitted.step(2200,100,true),"independently admitted continuing balance never receives unadmitted cleanup");
  for(unsigned fault=0;fault<5;++fault){
   Pipeline f(nose);f.acquire();f.command.owns=false;
   if(fault==0)++f.owner.rig;
   if(fault==1)f.producer.invalidate();
   check(!f.step(2190,fault==2?200u:fault==3?300u:100u,false,fault!=3,fault==4),"owner/reset/action-hop/bail/push boundary cancels stale cleanup");
   check(!f.step(2224),"cancelled token cannot later put down unrelated contact");
  }
 }
 std::cout<<checks<<" native acquisition boundary lifecycle checks passed\n";
}
