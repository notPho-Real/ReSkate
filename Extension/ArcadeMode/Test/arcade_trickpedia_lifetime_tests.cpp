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
struct Schema { unsigned hash; };
constexpr Schema page{0x5844be65};
constexpr unsigned text_field=2,button_field=3;
namespace arcade_mode { bool on{};bool enabled(){return on;} }
namespace menu_view { constexpr unsigned tools_page=1; }
struct State { Address manager{10};Value page{3,1}; } fixture_page;
const State& page_state(unsigned){return fixture_page;}
std::atomic<bool> game_in_menu{true};
std::atomic<bool> back_wanted{};
struct Back {
    int identity{};
    Address input{123};
    std::string navigation{"SP_Back"};
    bool active{true};
    bool operator==(const Back&) const = default;
};
struct Model {
    int focus{};
    Back back{};
    std::string title,content;
    bool operator==(const Model&) const = default;
};
struct Context {
    Address manager{10};
    mutable std::map<std::uint64_t,Model> models{{2,{0,{77},"Native","Flick controls"}},{3,{0,{88,123,"ToolsBack"},"Tools","Overlay shell"}}};
    mutable unsigned next{4};
    mutable unsigned creates{},copies{},destroys{};
    mutable unsigned fail_create{},fail_copy{};
    Address type(Schema) const {return 1;}
    Address type_of(std::uint64_t handle)const{return models.contains(handle)?1:0;}
    std::vector<Root> roots(std::initializer_list<std::uint32_t> hashes)const{
        if(hashes.size()!=1 || *hashes.begin()!=page.hash)return {};
        std::vector<Root> result;
        for(const auto& [handle,model]:models)result.push_back({{handle,1}});
        return result;
    }
    Value field(Value value,std::uint32_t key)const {
        if(key==276893451U)return {value.handle,3};
        if(key==0xa2b71c93)return {value.handle,6};
        if(key==0x3d00de27)return {value.handle,7};
        if(key==0xb1e14cd6)return {value.handle,8};
        if(key==0x4b3146e9)return {value.handle,9};
        throw std::runtime_error("unknown field");
    }
    Value path(Value value,std::initializer_list<std::uint32_t> keys)const {
        return {value.handle,*keys.begin()==0xfafda1a6?2u:*keys.begin()==0x10810f0b?5u:4u};
    }
    Address address(Value value)const {
        const auto& model=models.at(value.handle);
        if(value.type==2)return reinterpret_cast<Address>(&model.focus);
        if(value.type==3)return reinterpret_cast<Address>(&model.back);
        return reinterpret_cast<Address>(&model);
    }
    Value create(Schema,std::uint64_t)const {
        if(++creates==fail_create)throw std::runtime_error("injected create failure");
        const auto handle=next++;models[handle]={};return {handle,1};
    }
    void copy(Value target,Address from)const {
        if(++copies==fail_copy)throw std::runtime_error("injected publication failure");
        auto& model=models.at(target.handle);
        if(target.type==3)model.back=*reinterpret_cast<const Back*>(from);
        else model=*reinterpret_cast<const Model*>(from);
    }
    void set(Value value,int focus)const{models.at(value.handle).focus=focus;}
    void set(Value value,Address input)const{models.at(value.handle).back.input=input;}
    void set(Value value,bool active)const{models.at(value.handle).back.active=active;}
    std::string text(Value value)const{return models.at(value.handle).back.navigation;}
    void text(Value value,const std::string& text)const{
        if(value.type==8)models.at(value.handle).back.navigation=text;
        else models.at(value.handle).title=text;
    }
    void destroy(Value value)const {if(models.erase(value.handle))++destroys;}
};
template<class T>T read(Address at){return *reinterpret_cast<const T*>(at);}
#include "Extension/UI/NativeMenu/native_trickpedia.inl"
void require(bool value,const char* why){if(!value){std::cerr<<why<<'\n';std::exit(1);}}
void reset(){trickpedia_page={};trickpedia_seen=0;back_wanted=false;arcade_mode::on=true;game_in_menu=true;fixture_page.manager=10;}
}
int main(){
    reset();Context context;const auto original=context.models.at(2);
    // The live screen has no wrapper root. A simultaneously focused map page
    // must remain untouched, even when it is encountered before Skatepedia.
    reset();Context discovery;
    discovery.models[1]={0,{99,123,"MapBack"},"","Map"};
    const auto map=discovery.models.at(1);
    update_trickpedia(discovery,1000);
    require(discovery.models.at(2).title=="ARCADE TRICKPEDIA","direct page root was not discovered");
    require(discovery.models.at(1)==map,"another focused page was replaced");
    restore_trickpedia(discovery);
    for(unsigned reason=0;reason<4;++reason){
        reset();Context ignored;
        ignored.models.at(3).back.navigation="ToolsBack";
        if(reason==0)ignored.models.at(2).back.navigation="MapBack";
        if(reason==1)ignored.models.at(2).focus=-1;
        if(reason==2)arcade_mode::on=false;
        if(reason==3)game_in_menu=false;
        const auto before=ignored.models;
        update_trickpedia(ignored,1000);
        require(ignored.models==before && !ignored.creates && !trickpedia_seen.load(),
            "inactive or unrelated page acquired Trickpedia ownership");
    }
    reset();
    for(unsigned round=0;round<3;++round){
        arcade_mode::on=true;update_trickpedia(context,1000+round*1000);
        require(context.models.at(2).title=="ARCADE TRICKPEDIA","native entry did not switch");
        require(context.models.at(2).back==original.back,"original Back action replaced");
        require(context.models.size()==4,"retained copies missing");
        back_wanted=true;update_trickpedia(context,1001+round*1000);
        require(context.models.at(2).back.input==0 && context.models.at(2).back.navigation.empty() &&
            context.models.at(2).back.active,"search did not swallow native Back");
        if(round!=1) {
            back_wanted=false;update_trickpedia(context,1002+round*1000);
            require(context.models.at(2).back==original.back,"closing search replaced original Back navigation");
        }
        arcade_mode::on=false;update_trickpedia(context,1001+round*1000);
        back_wanted=false;
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
