#include "Extension/ArcadeMode/arcade_input_dispatch.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};void check(bool v,const char*w){++checks;if(!v){std::cerr<<"FAIL: "<<w<<'\n';std::exit(1);}}
struct Sink{unsigned manual{},nose{},other{};InputRequest last{};
 static InputDisposition receive(void*p,const InputRequest&r)noexcept{auto&s=*static_cast<Sink*>(p);s.last=r;if(r.action==InputAction::manual)++s.manual;else if(r.action==InputAction::nose_manual)++s.nose;else ++s.other;return InputDisposition::unsupported;}
 InputAdapter adapter(){return{this,receive};}};
void normalized_publication(){
 for(short y:{short{13056},short{13057},short{-12799},short{-12800}}){
  const auto state=InputEvents::normalize({0,0,y},77);
  const auto expected=y>=13057?8:y<=-12800?2:0;
  check(state.direction==expected && state.sampled_at==77 && state.available,"reference axis boundary is shared by published held state and command normalization");
 }
 for(unsigned bits=0;bits<16;++bits){
  const auto state=InputEvents::normalize({static_cast<std::uint16_t>(bits),25000,25000},100);
  const auto active=bits?bits:9u;
  check(state.direction==InputEvents::encode(active&8u?1:active&4u?-1:0,active&2u?-1:active&1u?1:0),"source direction priority and Dpad override are deterministic for every mask");
  auto controls=InputBalanceControls::vertical();const auto pair=state.configured(controls);
  check(pair.available && pair.sampled_at==100 && pair.first==((active&1u)!=0) && pair.second==((active&2u)!=0),
   "configured correction retains both cardinal pressed states independently of one eight-way command");
  controls.first={InputSource::buttons,13,InputEdge::press};controls.second={InputSource::triggers,1,InputEdge::press};
  const auto custom=InputEvents::normalize({static_cast<std::uint16_t>(bits|0x2000u),0,0,0,255},101).configured(controls);
  check(custom.first && custom.second && custom.sampled_at==101,"configured controls are descriptors, not hardcoded Manual token names");
  controls.admitted=false;check(!state.configured(controls).available,"missing control admission cannot become a configured mechanic input");
 }
 const auto diagonal=InputEvents::normalize({0,20000,25000},10);
 check(diagonal.direction==9 && diagonal.pressed({InputSource::direction,8,InputEdge::press}),
  "diagonal pressed Up component is a balance control, not an Up command event");
 const auto missing=InputEvents::normalize({0x2001,25000,25000,255,255},9,false);
 check(!missing.available && !missing.buttons && !missing.triggers && !missing.configured(InputBalanceControls::vertical()).available,
  "unavailable physical sample publishes no held input or inferred native authority");
 check(!diagonal.pressed({InputSource::direction,8,InputEdge::release}),"snapshot held state is not a fabricated release event");
}
void physical_capacity(){
 for(auto policy:{InputPolicy::production(),InputPolicy::compatibility()}){
  InputEvents input;input.sample(0,{},false,false,policy);
  for(unsigned i=1;i<=50;++i){input.sample(2*i-1,{0,0,25000},false,false,policy);input.sample(2*i,{},false,false,policy);}
  check(input.history().events().size()==100 && input.history().overflows()==0,"one hundred physical edges fit regardless of legacy held projections");
  std::uint64_t sequence=1;
  for(const auto&e:input.history().events())check(e.token.source==InputSource::direction && e.sequence==sequence++,"physical IDs have one shared ownership domain without duplicate projection records");
  input.sample(101,{0,0,25000},false,false,policy);
  check(input.history().overflows()==1 && input.history().events()[0].sequence==2,"only the101st physical edge overflows");
  input.discard_intent();input.sample(102,{0,0,25000},false,false,policy);
  check(input.history().events().empty() && input.digital_state().pressed({InputSource::direction,8,InputEdge::press}),"intent clearing retains held baseline without another physical press");
  input.sample(103,{},false,false,policy);
  check(input.history().events().size()==1 && input.history().events()[0].sequence==102 && input.history().events()[0].token.edge==InputEdge::release,
   "post-clear physical release retains immutable monotonic identity");
  input.reset();input.sample(200,{0,0,25000},false,false,policy);check(input.history().events().empty(),"new owner startup establishes held baseline only");
 }
}
void manual_false_pairs_and_ownership(){
 for(auto policy:{InputPolicy::production(),InputPolicy::compatibility(),InputPolicy::strict()}){
  UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};env.policy=policy;
  input.poll(0,{},env,sink.adapter());input.poll(100,{0,20000,25000},env,sink.adapter());input.poll(200,{0,0,-25000},env,sink.adapter());
  check(!sink.manual && !sink.nose,"diagonal-first cannot fabricate a cardinal Manual prefix under any supported profile");
  UnifiedInput consumed;Sink other;consumed.poll(0,{},env,other.adapter());consumed.poll(10,{0,0,-25000},env,other.adapter());consumed.poll(20,{},env,other.adapter());
  consumed.poll(30,{0,0,-25000},env,other.adapter());consumed.poll(40,{0x200,0,-25000},env,other.adapter());consumed.poll(50,{},env,other.adapter());consumed.poll(100,{0,0,25000},env,other.adapter());
  check(other.other==1 && !other.nose,"unsupported ordinary actor still cannot donate its consumed physical prefix to another Manual request");
 }
 for(bool fresh:{false,true}){
  UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};
  input.poll(0,fresh?PhysicalInputSample{}:PhysicalInputSample{0,0,25000},env,sink.adapter());
  input.poll(10,{2,0,25000},env,sink.adapter());
  if(fresh)input.poll(15,{2,0,-25000},env,sink.adapter());
  input.poll(20,{0,0,25000},env,sink.adapter());
  check(sink.nose==(fresh?1u:0u),"Dpad release cannot re-press an unchanged held stick but permits an actual fresh physical reversal");
 }
}
void pair_windows_and_sampling(){
 struct Window{InputPolicy policy;InputContext context;unsigned limit;bool inclusive;};
 const Window windows[]{{InputPolicy::production(),InputContext::ground,275,true},
  {InputPolicy::strict(),InputContext::ground,220,false},{InputPolicy::production(),InputContext::air,400,false}};
 for(const auto&w:windows)for(unsigned age:{w.limit-1,w.limit,w.limit+1})for(bool pad:{false,true}){
  UnifiedInput input;Sink sink;InputEnvironment env{w.context,true};env.policy=w.policy;
  input.poll(0,{},env,sink.adapter());input.poll(10,{static_cast<std::uint16_t>(pad?1:0),0,static_cast<std::int16_t>(pad?0:25000)},env,sink.adapter());
  input.poll(10+age,{static_cast<std::uint16_t>(pad?2:0),0,static_cast<std::int16_t>(pad?0:-25000)},env,sink.adapter());
  const bool expected=w.inclusive?age<=w.limit:age<w.limit;
  check(sink.manual==(expected?1u:0u),"220/400 reference queries and separately declared55ms ground allowance preserve exact boundaries");
 }
 for(std::uint64_t cadence:{17u,34u})for(std::uint64_t phase=0;phase<cadence;++phase)for(bool pad:{false,true})for(bool gap_held:{false,true}){
  UnifiedInput input;Sink sink;InputEnvironment env{InputContext::ground,true};
  for(std::uint64_t now=0;now<700;now+=cadence){
   const auto first=100+phase,second=first+250;
   PhysicalInputSample sample{};
   if(now<first)sample.buttons=0x2000;
   if(now>=first && now<second){sample.y=25000;if(pad){sample.y=0;sample.buttons=1;}}
   if(gap_held && now>=first && now<first+cadence)sample.buttons=static_cast<std::uint16_t>(sample.buttons|0x2000u);
   if(!pad && !gap_held && now>=first+2*cadence && now<first+3*cadence)sample.x=20000;
   if(now>=second){sample.y=-25000;if(pad){sample.y=0;sample.buttons=2;}}
   input.poll(now,sample,env,sink.adapter());input.poll(now,sample,env,sink.adapter());
  }
  if(sink.manual!=(gap_held?0u:1u))std::cerr<<"cadence="<<cadence<<" phase="<<phase<<" pad="<<pad<<" gap="<<gap_held<<" manual="<<sink.manual<<"\n";
  check(sink.manual==(gap_held?0u:1u),"source cardinal first edge survives B release and intermediate diagonal travel at30/60Hz; held-through-gap does not become a press");
 }
}
void transaction_queue_regressions(){
 InputHistory history;const InputToken a{InputSource::buttons,13,InputEdge::press},b{InputSource::buttons,14,InputEdge::press};
 history.record(a,0);const std::array pair{a,b};check(!history.match({pair,300},1) && !history.events()[0].owners,"failed query leaves no maybe-used footprint");
 history.record(b,2);const auto match=history.match({pair,300},2);check(history.consume(match,InputConsumption::extra),"successful query commits exact physical IDs atomically");
 history.restore(InputConsumption::extra,101,100);check(history.events()[0].owners && !history.events()[1].owners,"restore only changes the requested category within its original-time window");
 InputRequestQueue queue;queue.push({InputAction::manual,InputContext::air,10,710,1});queue.push({InputAction::rail_select,InputContext::air,10,1000,1});queue.push({InputAction::flip,InputContext::air,10,0,1});
 queue.clear_ordinary();check(queue.slots()[0].action==InputAction::manual && queue.slots()[InputRequestQueue::capacity+1].action==InputAction::rail_select,"ordinary table replacement preserves independent Manual and extra-grind slots");
 for(unsigned i=0;i<100;++i)check(queue.push({InputAction::flip,InputContext::air,10,0,1}),"ordinary queue retains bounded capacity");
 queue.cancel(InputAction::flip);check(queue.push({InputAction::grab,InputContext::air,10,0,1}),"frozen636/f53 cancellation-capacity fix remains active");
}
}
int main(){normalized_publication();physical_capacity();manual_false_pairs_and_ownership();pair_windows_and_sampling();transaction_queue_regressions();std::cout<<checks<<" physical input state checks passed\n";}
