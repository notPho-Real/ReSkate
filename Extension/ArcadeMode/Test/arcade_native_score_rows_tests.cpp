// Production native-history augmentation, with typed model/stack substitutes.
#include "Extension/UI/score_line_hud.h"
#include "Engine/Game/Build/20260929/score_hud.h"
#include "Engine/Game/Build/20260929/hud_corner.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <span>
#include <stdexcept>
namespace dingosdk::tricks::feed {
constexpr std::size_t most_owned=16;
std::vector<std::uint64_t> entries;
void own(std::vector<std::uint64_t> ids){entries=std::move(ids);}
}
namespace dingosdk::score_line {
namespace score=game::build::v20260929::score_hud;
namespace corner=game::build::v20260929::hud_corner;
using Address=std::uintptr_t;
struct Value {std::uint64_t handle{},type{};};
struct Ref {Address record{};std::uint64_t handle{};};
struct Widget {Address blueprint{};Ref data;};
namespace menu_data { struct Schema{unsigned size{};};constexpr Schema stack_item{48}; }
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Context {
    Address manager{1};mutable unsigned made{},destroyed{},published{};mutable bool fail_after_publish{},fail_make{},fail_label_copy{},fail_transition_copy{};
    mutable std::map<std::uint64_t,std::string> names{{20,"Manual"},{21,"Kickflip"},{22,"Heelflip"}};
    mutable std::map<std::uint64_t,unsigned> label_styles{{20,42},{21,43},{22,44}};
    mutable std::map<std::uint64_t,std::vector<std::byte>> items;
    mutable std::vector<std::byte> stack;
    unsigned stack_transition{91};
    bool root_live{true};
    Address type_of(std::uint64_t h)const{return h==1?(root_live?100u:0u):names.contains(h)?200u:items.contains(h)?300u:0u;}
    Value path(Value v,std::initializer_list<std::uint32_t> fields)const{
        if(fields.size()==2 && *std::next(fields.begin())==corner::default_transition){
            require(v.handle==1 && *fields.begin()==score::trick_list,"read transition from the wrong stack");
            return {1,500};
        }
        return v;
    }
    Value field(Value v,std::uint32_t hash)const{
        if(hash==corner::item_transition){require(v.type==300,"transition target is not an owned stack item");return {v.handle,500};}
        require(hash==score::row_name,"copied a field outside the name label");return {v.handle,400};
    }
    Address address(Value v)const{require(v.type==400 || (v.type==500 && v.handle==1),"copied a complete trick row");return static_cast<Address>(v.handle);}
    void copy(Value v,Address source)const{
        if(v.type==500){
            require(v.handle>=65536 && source==1,"transition copy touched native ownership");
            auto& bytes=items.at(v.handle);std::memcpy(bytes.data()+44,&stack_transition,4);
            if(fail_transition_copy){fail_transition_copy=false;throw std::runtime_error("partial transition initialization");}
            return;
        }
        require(v.type==400 && v.handle>=65536 && source<65536,"label copy touched native ownership");
        names[v.handle]=names.at(source);label_styles[v.handle]=label_styles.at(source);
        if(fail_label_copy){fail_label_copy=false;throw std::runtime_error("partial label initialization");}
    }
    void text(Value v,const std::string& text)const{require(v.handle>=65536,"rewrote native label");names[v.handle]=text;}

    std::string text(Value v,unsigned)const{return names.at(v.handle);}
    std::vector<std::byte> array(Value,unsigned max,unsigned& count,unsigned& stride)const{stride=48;count=static_cast<unsigned>(stack.size()/stride);require(count<=max,"overflow");return stack;}
    void array(Value,std::span<const std::byte> b,unsigned n)const{require(b.size()==std::size_t{n}*48,"stride");stack.assign(b.begin(),b.end());++published;if(fail_after_publish){fail_after_publish=false;throw std::runtime_error("partial native publication");}}
    void native(std::initializer_list<std::uint64_t> ids)const{stack.clear();for(auto id:ids){std::array<std::byte,48> bytes{};const Widget w{900,{0,id}};std::memcpy(bytes.data(),&w,sizeof w);bytes[40]=std::byte{77};stack.insert(stack.end(),bytes.begin(),bytes.end());}}
};
struct Row {std::string name;Value model,item;};
struct Hud {Value game_root{1,100};unsigned content_at{};Address row_blueprint{900};};
bool alive(const Context& c,Value v){return v.handle && c.type_of(v.handle)==v.type;}
Value label_text(const Context&,Value v,std::uint32_t){return v;}
Address row_type_now(Hud&,const Context&,double){return 200;}
Row make_row(Hud&,const Context& c,Address,const std::string& name){
    if(c.fail_make)throw std::runtime_error("allocation failed");
    const std::uint64_t id=65536ull*(++c.made);c.names[id]=name;c.label_styles[id]=0;std::vector<std::byte> bytes(48);
    const Widget widget{900,{0,id}};std::memcpy(bytes.data(),&widget,sizeof widget);c.items[id+1]=bytes;return {name,{id,200},{id+1,300}};
}
std::vector<std::byte> bytes_of(const Context& c,Value value){return c.items.at(value.handle);}
void destroy_quietly(const Context& c,Value& v){require(v.handle>=65536,"destroyed a native model");c.names.erase(v.handle);c.label_styles.erase(v.handle);c.items.erase(v.handle);++c.destroyed;v={};}
#include "Extension/UI/score_line_native_rows.inl"
std::vector<std::string> shown(const Context& c){std::vector<std::string> out;for(std::size_t i=0;i<c.stack.size();i+=48){Widget w{};std::memcpy(&w,c.stack.data()+i,sizeof w);out.push_back(c.names.at(w.data.handle));}return out;}
void request(std::vector<ExtraTrick> rows,double now){auto& n=native_rows();n.requested=std::move(rows);n.requested_root=1;n.at=now;}
}
int main(){using namespace dingosdk::score_line;
    Hud h;Context c;c.native({20,21});const auto original=c.stack;
    request({{1,"Casper",1}},1000);update_native_rows(h,c,true,1000);
    require(shown(c)==std::vector<std::string>{"Casper","Manual","Kickflip"},"custom row missing or native order changed");
    unsigned transition{};std::memcpy(&transition,c.stack.data()+44,4);
    require(transition==c.stack_transition,"custom row retained the unrelated d-pad transition");
    require(std::equal(original.begin(),original.end(),c.stack.begin()+48),"native stack item bytes changed");
    require(c.names.at(20)=="Manual" && c.names.at(21)=="Kickflip","rewrote native row labels");
    require(c.label_styles.at(native_rows().rows.front().row.model.handle)==42,"custom row did not retain the native name label styling");
    require(c.label_styles.at(20)==42 && c.label_styles.at(21)==43,"native label styling changed");
    const auto made=c.made,published=c.published;update_native_rows(h,c,true,1010);
    require(c.made==made && c.published==published,"unchanged native history rebuilt every tick");
    // A bound widget resolves its references and advances its own state after
    // publication. Neither an idle tick nor a new row may reset that state.
    Widget live{};std::memcpy(&live,c.stack.data(),sizeof live);
    live.data.record=12345;std::memcpy(c.stack.data(),&live,sizeof live);
    c.stack[40]=std::byte{63};
    const unsigned resolved_transition=123;std::memcpy(c.stack.data()+44,&resolved_transition,4);
    const std::vector<std::byte> live_item(c.stack.begin(),c.stack.begin()+48);
    update_native_rows(h,c,true,1015);
    require(c.published==published && std::equal(live_item.begin(),live_item.end(),c.stack.begin()),
        "live custom item state was reset from its creation template");
    request({{1,"Casper",1},{2,"Casper Flip",1}},1020);update_native_rows(h,c,true,1020);
    require(shown(c)==std::vector<std::string>{"Casper Flip","Casper","Manual","Kickflip"},"custom chronological ordering lost");
    require(std::equal(live_item.begin(),live_item.end(),c.stack.begin()+48),
        "adding a custom trick reset an existing live item");
    c.native({22,20,21});const auto refreshed=c.stack;update_native_rows(h,c,true,1030);
    require(shown(c)==std::vector<std::string>{"Heelflip","Casper Flip","Casper","Manual","Kickflip"},"new native trick not above preceding custom tricks");
    c.names[20]="Pop Shove-It";c.native({20,21});update_native_rows(h,c,true,1040);
    require(shown(c)==std::vector<std::string>{"Pop Shove-It","Casper Flip","Casper","Kickflip"},"recycled native row used stale text ordering");
    c.native({22,20,21});update_native_rows(h,c,true,1050);update_native_rows(h,c,false,1060);
    require(c.stack==refreshed && c.names.contains(20) && c.names.contains(21) && c.names.contains(22),"release did not preserve the latest exact native rows");
    require(c.items.empty() && c.names.size()==3,"release leaked owned row models");
    request({{1,"Casper",2}},2000);update_native_rows(h,c,true,2000);
    request({{1,"Casper",3}},2010);update_native_rows(h,c,true,2010);
    require(c.made==4 && shown(c).front()=="Casper","new combo reused the previous combo's occurrence");
    update_native_rows(h,c,true,2010+lease_ms+1);require(c.items.empty(),"expired lease retained custom rows");
    request({{4,"Casper",4}},3000);c.fail_after_publish=true;
    try{update_native_rows(h,c,true,3000);}catch(const std::runtime_error&){}
    update_native_rows(h,c,false,3010);require(c.stack==refreshed && c.items.empty(),"partially published row destroyed before removing its reference");
    request({{5,"Casper",5}},4000);c.fail_make=true;
    try{update_native_rows(h,c,true,4000);}catch(const std::runtime_error&){}
    c.fail_make=false;clear_native_rows(h,c);require(c.stack==refreshed && c.items.empty(),"failed allocation changed native history");
    request({{8,"Casper",5}},4500);c.fail_label_copy=true;
    try{update_native_rows(h,c,true,4500);}catch(const std::runtime_error&){}
    update_native_rows(h,c,false,4510);require(c.stack==refreshed && c.items.empty(),"partial label initialization leaked a row or changed native history");
    request({{9,"Casper",5}},4600);c.fail_transition_copy=true;
    try{update_native_rows(h,c,true,4600);}catch(const std::runtime_error&){}
    update_native_rows(h,c,false,4610);require(c.stack==refreshed && c.items.empty(),"partial transition initialization leaked a row or changed native history");
    request({{6,"Casper",6}},5000);update_native_rows(h,c,true,5000);Context other;other.manager=2;other.native({21});
    request({{7,"Casper Flip",7}},5010);update_native_rows(h,other,true,5010);update_native_rows(h,other,false,5020);
    require(shown(other)==std::vector<std::string>{"Kickflip"},"old registry history was copied into the new registry");
    std::cout<<"Native trick history preservation, ordering, expiry and partial-publication cleanup passed\n";
}
