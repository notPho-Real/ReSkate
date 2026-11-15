// Exercise the production override with a typed, retaining native UI substitute.
#include "Extension/UI/hud_corner.h"
#include "Extension/UI/own_ui_writes.h"
#include "Extension/UI/score_line.h"
#include "Engine/Game/Build/20260929/score_hud.h"
#include "Engine/Game/Build/20260929/engine.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
namespace dingosdk {
namespace addr = game::build::v20260929;
namespace memory {
bool peek_bytes(std::uintptr_t p, void* out, std::size_t n) { if(!p)return false; std::memcpy(out,reinterpret_cast<const void*>(p),n);return true; }
}
namespace game {
using Address=std::uintptr_t;
unsigned strings{},released{}; bool fail_assign{};
void* assign(void* out,const char* text,std::uint32_t n) {
    if(fail_assign){*static_cast<Address*>(out)=0;return out;}
    auto* p=new char[std::size_t{n}+1]; std::memcpy(p,text,n);p[n]=0;
    *static_cast<Address*>(out)=reinterpret_cast<Address>(p);++strings;return out;
}
void release(Address* p){delete[] reinterpret_cast<char*>(*p);*p=0;++released;}
struct Data {struct {decltype(&game::assign) assign{&game::assign};} values;};
Data& native_data(){static Data d;return d;}
}
namespace multiplayer::menu_data {
using Address=std::uintptr_t;
struct Value {std::uint64_t handle{};Address type{};};
struct Ref {Address record{};std::uint64_t handle{};};
struct Member {unsigned offset{};Address type{};};
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
unsigned kind(Address t){return t==100 || t==101 || t==102 ? 2u : static_cast<unsigned>(t);}
unsigned size(Address t){if(t==100)return 128;if(t==101)return 24;if(t==102)return 16;if(t==7)return 8;if(t==10)return 1;return 4;}
template<class T>T read(Address p){T v{};std::memcpy(&v,reinterpret_cast<const void*>(p),sizeof v);return v;}
std::string string(Address p,std::size_t maximum){if(!p)return {};std::string s(reinterpret_cast<const char*>(p));require(s.size()<=maximum,"oversized text");return s;}
struct Context {
    Address base{reinterpret_cast<Address>(&game::release)-addr::engine::native_text_release},manager{1};
    mutable std::array<std::byte,128> data{};
    mutable std::map<unsigned,std::string> texts;
    bool alive{true},wrong_type{}; mutable unsigned publishes{},fail_publish{};
    Context(){set_raw<float>(0,1.5f);set_raw<float>(4,22);set_raw<std::uint8_t>(56,0);set_raw<float>(60,.4f);
        set_raw<Ref>(64,{99,3});store_text(8,"22");store_text(32,"1.5");set_raw<std::uint32_t>(100,123456);}
    template<class T>void set_raw(unsigned offset,T v)const{std::memcpy(data.data()+offset,&v,sizeof v);}
    template<class T>T get(unsigned offset)const{return read<T>(reinterpret_cast<Address>(data.data()+offset));}
    void store_text(unsigned offset,const std::string& s)const{texts[offset]=s;set_raw<Address>(offset,reinterpret_cast<Address>(texts[offset].c_str()));}
    static unsigned offset(std::uint64_t handle){return handle==1?0u:static_cast<unsigned>(handle-1000);}
    Address type_of(std::uint64_t h)const{
        if(!alive)return 0;if(h==1)return 100;
        switch(offset(h)){case 0:case 4:case 60:return wrong_type?16:19;case 8:case 32:return 7;case 56:return 10;case 64:return 102;case 112:case 113:return 101;default:return 0;}
    }
    Member member(Address type,std::uint32_t hash)const {
        if(type==101 && hash==addr::score_hud::label_text)return {0,7};
        require(type==100,"unexpected parent");
        switch(hash){case addr::score_hud::multiplier:return {0,19};case addr::score_hud::line_score:return {4,19};
        case addr::score_hud::action_points:return {8,101};case addr::score_hud::multiplier_text:return {32,101};
        case addr::score_hud::ring_on:return {56,10};case addr::score_hud::action_fill:return {60,19};
        case addr::score_hud::scoring_data:return {64,102};default:throw std::runtime_error("unexpected field");}
    }
    Value field(Value parent,std::uint32_t hash)const {
        const auto m=member(parent.type,hash);
        if(parent.handle==1112)return {1008,7};if(parent.handle==1113)return {1032,7};
        if(m.type==101)return {m.offset==8?1112u:1113u,101};
        return {1000+m.offset,m.type};
    }
    Address address(Value v)const{require(alive,"expired owner");const auto at=v.handle==1112?8u:v.handle==1113?32u:offset(v.handle);return reinterpret_cast<Address>(data.data()+at);}
    std::string text(Value v,std::size_t maximum)const{return string(read<Address>(address(v)),maximum);}
    void text(Value v,const std::string& s)const;
    void publish(Value v,const void* source)const;
};
}
namespace hud_corner {
namespace menu_data=multiplayer::menu_data;
using Address=std::uintptr_t;using Value=std::uint64_t;
using ModelWrite=std::uint64_t (*)(Address,std::uint64_t,Address,const void*,std::uint8_t);
thread_local bool publishing{};
#include "Extension/UI/native_score_presentation.inl"
}
}
using namespace dingosdk;
using multiplayer::menu_data::Context;
const Context* current{};
std::uint64_t original(std::uintptr_t,std::uint64_t handle,std::uintptr_t type,const void* bytes,std::uint8_t){
    auto& c=*current;const auto at=handle==1112?8u:handle==1113?32u:Context::offset(handle);
    if(type==100){
        std::array<std::byte,128> incoming;std::memcpy(incoming.data(),bytes,incoming.size());
        std::array<std::string,2> strings;
        for(unsigned i=0;i<2;++i){std::uintptr_t p{};std::memcpy(&p,incoming.data()+(i?32:8),sizeof p);strings[i]=p?reinterpret_cast<const char*>(p):"";}
        c.data=incoming;c.store_text(8,strings[0]);c.store_text(32,strings[1]);
    }else if(type==7 || type==101){std::uintptr_t p{};std::memcpy(&p,bytes,sizeof p);if(p)c.store_text(at,reinterpret_cast<const char*>(p));else c.set_raw<std::uintptr_t>(at,0);}
    else std::memcpy(c.data.data()+at,bytes,multiplayer::menu_data::size(type));
    return 77;
}
void Context::publish(multiplayer::menu_data::Value v,const void* source)const{
    if(++publishes==fail_publish)throw std::runtime_error("injected publication failure");
    current=this;std::uint64_t result{};
    if(!hud_corner::publishing && hud_corner::score_presentation::write(manager,v.handle,v.type,source,0,&original,result))return;
    original(manager,v.handle,v.type,source,0);
}
void Context::text(multiplayer::menu_data::Value v,const std::string& s)const{const auto p=reinterpret_cast<std::uintptr_t>(s.c_str());publish(v,&p);}
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int main(){
    using namespace hud_corner;namespace p=score_presentation;
    Context c;const multiplayer::menu_data::Value root{1,100};
    ScorePresentation wanted{1,1,3,50,150,3};
    p::apply(c,root,wanted);
    check(c.get<float>(0)==3 && c.get<float>(4)==150 && c.text({1008,7},100)=="50" && c.text({1032,7},100)=="3","original widget numbers not updated");
    check(c.get<std::uint32_t>(100)==123456 && c.get<multiplayer::menu_data::Ref>(64).handle==3,"unrelated history or native scoring owner touched");
    float f=2;c.publish({1000,19},&f);c.text({1032,7},"2");
    check(c.get<float>(0)==3 && c.text({1032,7},100)=="3","native writes replaced the held multiplier");
    p::apply(c,root,std::nullopt);
    check(c.get<float>(0)==2 && c.text({1032,7},100)=="2" && c.get<float>(4)==22,"release restored old values instead of latest native values");
    check(c.text({1008,7},100)=="22" && !c.get<std::uint8_t>(56),"labels or ring not restored");
    p::apply(c,root,wanted);
    Context native;native.set_raw<float>(0,2.5f);native.set_raw<float>(4,90);native.store_text(8,"36");native.store_text(32,"2.5");native.set_raw<std::uint32_t>(100,9876);
    c.publish(root,native.data.data());
    check(c.get<float>(0)==3 && c.text({1032,7},100)=="3" && c.get<std::uint32_t>(100)==9876,"whole-model override did not preserve native unrelated fields");
    p::apply(c,root,std::nullopt);
    check(c.get<float>(0)==2.5f && c.text({1008,7},100)=="36" && c.get<std::uint32_t>(100)==9876,"whole-model write not restored");
    p::apply(c,root,wanted);std::array<std::uintptr_t,3> label{reinterpret_cast<std::uintptr_t>("4"),0,0};c.publish({1113,101},label.data());
    check(c.text({1032,7},100)=="3","parent label write bypassed override");p::apply(c,root,std::nullopt);
    check(c.text({1032,7},100)=="4","parent label native text lost on release");
    p::apply(c,root,wanted);std::uintptr_t empty{};c.publish({1032,7},&empty);p::apply(c,root,std::nullopt);
    check(c.get<std::uintptr_t>(32)==0,"native null string restored a stale pointer");
    p::apply(c,root,wanted);auto other=wanted;other.scoring=99;p::apply(c,root,other);
    check(c.get<float>(0)==2.5f && !p::state().root.handle,"scoring owner change did not release");
    p::apply(c,root,wanted);Context next;next.manager=2;auto next_wanted=wanted;next_wanted.manager=2;p::apply(next,root,next_wanted);p::apply(next,root,std::nullopt);
    check(next.get<float>(0)==1.5f,"old registry values leaked into replacement registry");
    for(unsigned failure=1;failure<=6;++failure){Context failed;failed.fail_publish=failure;try{p::apply(failed,root,wanted);}catch(const std::runtime_error&){}failed.fail_publish=0;p::apply(failed,root,std::nullopt);
        check(failed.get<float>(0)==1.5f && failed.text({1008,7},100)=="22","partial publication could not restore original fields");}
    check(game::strings==game::released,"intercepted engine strings leaked");
    std::cout<<"Original score widget field, parent, ownership and restoration tests passed\n";
}
