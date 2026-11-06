// Exercise the production native interception with a typed, retaining model test double.
// This validates ownership/restoration logic; only hardware proves the game's focus ABI.
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
using Address=std::uintptr_t;
struct Value { std::uint64_t handle{},type{}; };
struct Root { Value model; };
constexpr unsigned page=1,text_field=2;
namespace arcade_mode { bool on{};bool enabled(){return on;} }
namespace menu_view { constexpr unsigned tools_page=1; }
struct State { Address manager{10};Value page{3,1}; } source;
const State& page_state(unsigned){return source;}
std::atomic<bool> game_in_menu{true};
struct Model {
    int focus{},back{};
    std::string title,content;
    bool operator==(const Model&) const = default;
};
struct Context {
    Address manager{10};
    mutable std::map<std::uint64_t,Model> models{{2,{0,77,"Native","Flick controls"}},{3,{0,88,"Tools","Overlay shell"}}};
    mutable unsigned next{4};
    mutable unsigned creates{},copies{},destroys{};
    mutable unsigned fail_create{},fail_copy{};
    Address type(unsigned) const {return 1;}
    Address type_of(std::uint64_t handle)const{return handle==1?9:models.contains(handle)?1:0;}
    std::vector<Root> roots(std::initializer_list<std::uint32_t>)const{return {{{1,9}}};}
    Value field(Value value,std::uint32_t key)const {
        if(value.handle==1 && key==1244125067U)return {2,1};
        if(key==276893451U)return {value.handle,3};
        throw std::runtime_error("unknown field");
    }
    Value path(Value value,std::initializer_list<std::uint32_t> keys)const {
        return {value.handle,*keys.begin()==0xfafda1a6?2u:4u};
    }
    Address address(Value value)const {
        const auto& model=models.at(value.handle);
        if(value.type==2)return reinterpret_cast<Address>(&model.focus);
        if(value.type==3)return reinterpret_cast<Address>(&model.back);
        return reinterpret_cast<Address>(&model);
    }
    Value create(unsigned,std::uint64_t)const {
        if(++creates==fail_create)throw std::runtime_error("injected create failure");
        const auto handle=next++;models[handle]={};return {handle,1};
    }
    void copy(Value target,Address from)const {
        if(++copies==fail_copy)throw std::runtime_error("injected publication failure");
        auto& model=models.at(target.handle);
        if(target.type==3)model.back=*reinterpret_cast<const int*>(from);
        else model=*reinterpret_cast<const Model*>(from);
    }
    void set(Value value,int focus)const{models.at(value.handle).focus=focus;}
    void text(Value value,const std::string& text)const{models.at(value.handle).title=text;}
    void destroy(Value value)const {if(models.erase(value.handle))++destroys;}
};
template<class T>T read(Address at){return *reinterpret_cast<const T*>(at);}
#include "Extension/UI/NativeMenu/native_trickpedia.inl"
void require(bool value,const char* why){if(!value){std::cerr<<why<<'\n';std::exit(1);}}
void reset(){trickpedia_page={};trickpedia_seen=0;arcade_mode::on=true;game_in_menu=true;source.manager=10;}
}
int main(){
    reset();Context context;const auto original=context.models.at(2);
    for(unsigned round=0;round<3;++round){
        arcade_mode::on=true;update_trickpedia(context,1000+round*1000);
        require(context.models.at(2).title=="ARCADE TRICKPEDIA","native entry did not switch");
        require(context.models.at(2).back==original.back,"original Back action replaced");
        require(context.models.size()==4,"retained copies missing");
        arcade_mode::on=false;update_trickpedia(context,1001+round*1000);
        require(context.models.at(2)==original,"same-session original page not restored");
        require(context.models.size()==2 && !trickpedia_seen.load(),"menu ownership leaked after OFF");
    }
    for(unsigned failure=1;failure<=4;++failure){
        reset();Context c;c.fail_copy=failure;
        try{update_trickpedia(c,1000);}catch(const std::runtime_error&){}
        require(c.models.at(2)==original && c.models.size()==2,"partial publication did not unwind");
    }
    for(unsigned failure=1;failure<=2;++failure){
        reset();Context c;c.fail_create=failure;
        try{update_trickpedia(c,1000);}catch(const std::runtime_error&){}
        require(c.models.at(2)==original && c.models.size()==2,"partial allocation leaked");
    }
    reset();Context c;update_trickpedia(c,1000);game_in_menu=false;update_trickpedia(c,1010);
    require(c.models.at(2)==original && !trickpedia_seen.load(),"closing native menu leaked replacement");
    reset();update_trickpedia(c,2000);c.models.erase(2);update_trickpedia(c,2010);
    require(!trickpedia_page.manager && c.models.size()==1,"expired native page accessed or retained");
    reset();Context d;update_trickpedia(d,3000);d.manager=11;update_trickpedia(d,3010);
    require(!trickpedia_page.manager && !trickpedia_seen.load(),"stale manager retained overlay ownership");
    std::cout<<"Native Trickpedia lease: ON/OFF, Back, allocation/publication failures, close, expiry and manager replacement passed\n";
}
