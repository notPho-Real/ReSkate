// Production card adapter with typed, retaining native model substitutes.
// Rendered layout and the live ABI still require device acceptance.
#include "Extension/ArcadeMode/arcade_trickpedia.h"
#include "Engine/Game/Input/controller_bindings.h"
#include "Engine/Game/Build/20260929/skatepedia.h"
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
namespace dingosdk::arcade_mode {
bool on{};
bool enabled(){return on;}
ArcadeTrick grab_selection{ArcadeTrick::double_grab};
ArcadeTrick selected_grab(){return grab_selection;}
}
namespace {
using namespace dingosdk;
namespace addr=dingosdk::game::build::v20260929;
using Address=std::uintptr_t;
struct Value {std::uint64_t handle{},type{};unsigned index{};};
struct Schema {unsigned hash{},size{};};
struct Root {Value model;};
constexpr Schema page{0x5844be65,1968};
std::atomic<bool> game_in_menu{true};
ControllerStyle controller_style{};
void DingoSDKOverlayReadControllerInput(ControllerInput* input,bool){*input={};input->style=controller_style;}
struct Model {
    unsigned type{1};
    int focus{};
    std::string back{"SP_Back"},title{"Native"},content{"Native page and preview"};
    std::vector<std::string> inputs;
    bool operator==(const Model&) const=default;
};
struct Context {
    Address manager{10};
    mutable std::map<std::uint64_t,Model> models{
        {2,{}},{3,{2,0,"","ID_SKATEPEDIA_OLLIE","Original description",{"Flick up"}}},
        {4,{2,0,"","","",{}}}};
    mutable unsigned next{5},creates{},copies{},destroys{},fail_create{},fail_copy{};
    Address type(Schema s)const{return s.hash==page.hash?1:2;}
    Address type_of(std::uint64_t h)const {const auto it=models.find(h);return it==models.end()?0:it->second.type;}
    std::vector<Root> roots(std::initializer_list<std::uint32_t> hashes)const {
        std::vector<Root> out;
        for(const auto& [h,m]:models)if(m.type==type({*hashes.begin(),0}))out.push_back({{h,m.type,0}});
        return out;
    }
    Value field(Value v,std::uint32_t key)const {
        if(key==addr::skatepedia::inputs)return {v.handle,10,0};
        if(key==addr::skatepedia::text && v.type==11)return v;
        throw std::runtime_error("unexpected field write");
    }
    Value path(Value v,std::initializer_list<std::uint32_t> keys)const {
        const auto k=*keys.begin();
        if(k==addr::skatepedia::focus)return {v.handle,20,0};
        if(k==addr::skatepedia::back)return {v.handle,21,0};
        if(k==addr::skatepedia::title)return {v.handle,22,0};
        throw std::runtime_error("unexpected path");
    }
    Value element(Value v,unsigned i)const {(void)models.at(v.handle).inputs.at(i);return {v.handle,11,i};}
    Address address(Value v)const {
        const auto& m=models.at(v.handle);
        if(v.type==20)return reinterpret_cast<Address>(&m.focus);
        if(v.type==10)return reinterpret_cast<Address>(&m.inputs);
        return reinterpret_cast<Address>(&m);
    }
    std::string text(Value v,std::size_t =128)const {
        const auto& m=models.at(v.handle);
        if(v.type==21)return m.back;
        if(v.type==22)return m.title;
        if(v.type==11)return m.inputs.at(v.index);
        throw std::runtime_error("unexpected text read");
    }
    void text(Value v,const std::string& text)const {
        if(v.type!=11)throw std::runtime_error("changed a field outside the input panel");
        models.at(v.handle).inputs.at(v.index)=text;
    }
    Value create(Schema s,std::uint64_t)const {
        if(++creates==fail_create)throw std::runtime_error("injected allocation failure");
        const auto h=next++;Model m;m.type=static_cast<unsigned>(type(s));models[h]=m;return {h,m.type,0};
    }
    void copy(Value v,Address from)const {
        if(++copies==fail_copy)throw std::runtime_error("injected copy failure");
        if(v.type==10)models.at(v.handle).inputs=*reinterpret_cast<const std::vector<std::string>*>(from);
        else models.at(v.handle)=*reinterpret_cast<const Model*>(from);
    }
    std::vector<std::byte> array(Value v,unsigned maximum,unsigned& count,unsigned& stride)const {
        const auto& source=models.at(v.handle).inputs;
        count=static_cast<unsigned>(source.size());stride=sizeof(const std::string*);
        if(count>maximum)throw std::runtime_error("array bound");
        std::vector<std::byte> bytes(count*stride);
        for(unsigned i=0;i<count;++i){const auto* text=&source[i];std::memcpy(bytes.data()+i*stride,&text,stride);}
        return bytes;
    }
    void array(Value v,std::span<const std::byte> source,unsigned count)const {
        std::vector<std::string> retained;
        for(unsigned i=0;i<count;++i) {
            // Mirror native typed retain before replacing the old array.
            const std::string* text{};
            std::memcpy(&text,source.data()+i*sizeof(text),sizeof(text));
            retained.push_back(*text);
        }
        models.at(v.handle).inputs=std::move(retained);
    }
    void destroy(Value v)const {if(models.erase(v.handle))++destroys;}
};
template<class T>T read(Address p){return *reinterpret_cast<const T*>(p);}
#include "Extension/UI/NativeMenu/native_trickpedia.inl"
void require(bool ok,const char* why){if(!ok){std::cerr<<why<<'\n';std::exit(1);}}
void reset(){trickpedia_page={};arcade_mode::on=true;game_in_menu=true;controller_style=ControllerStyle::xbox;
    arcade_mode::grab_selection=arcade_mode::ArcadeTrick::double_grab;}
}
int main(){
    reset();Context c;const auto original=c.models;
    update_trickpedia(c,1000);
    require(c.models.at(2)==original.at(2),"native page or navigation changed");
    require(c.models.at(3).title==original.at(3).title && c.models.at(3).content==original.at(3).content,
        "title, description or preview changed");
    require(c.models.at(3).inputs.size()==1 && c.models.at(3).inputs.front().find("hold A")!=std::string::npos,
        "native input panel did not receive the existing binding");
    require(c.models.at(3).inputs.front().find("Experimental")!=std::string::npos,"status missing");
    const auto copies=c.copies;update_trickpedia(c,1010);require(c.copies==copies,"unchanged panel recopied every tick");
    arcade_mode::on=false;update_trickpedia(c,1020);
    require(c.models==original && !trickpedia_page.manager,"OFF did not restore exact input guidance");
    reset();Context style;update_trickpedia(style,1000);
    controller_style=ControllerStyle::dualsense;update_trickpedia(style,1010);
    require(style.models.at(3).inputs.front().find("Cross")!=std::string::npos,
        "controller change left Xbox inputs on the open card");
    arcade_mode::grab_selection=arcade_mode::ArcadeTrick::nosegrab;update_trickpedia(style,1020);
    require(trickpedia_page.selected_grab==arcade_mode::ArcadeTrick::nosegrab,
        "selected grab change did not refresh open guidance");
    arcade_mode::on=false;update_trickpedia(style,1030);
    require(style.models==original,"controller refresh restored another override instead of native inputs");
    for(unsigned reason=0;reason<4;++reason){
        reset();Context ignored;
        if(reason==0)ignored.models.at(2).back="MapBack";
        if(reason==1)ignored.models.at(2).focus=-1;
        if(reason==2)arcade_mode::on=false;
        if(reason==3)game_in_menu=false;
        const auto before=ignored.models;update_trickpedia(ignored,1000);
        require(ignored.models==before && !ignored.creates,"inactive or unrelated page modified");
    }
    reset();Context selection;update_trickpedia(selection,1000);
    selection.models.at(3).title="ID_SKATEPEDIA_KICKFLIP";selection.models.at(3).inputs={"Flick diagonally"};
    update_trickpedia(selection,1010);
    require(selection.models.at(3).inputs.front().find("X")!=std::string::npos,"new selection did not refresh inputs");
    arcade_mode::on=false;update_trickpedia(selection,1020);
    require(selection.models.at(3).inputs==std::vector<std::string>{"Flick diagonally"},"OFF restored previous trick inputs");
    reset();Context variants;update_trickpedia(variants,1000);
    variants.models.at(3).title="ID_SKATEPEDIA_NOLLIE_KICKFLIP";
    variants.models.at(3).inputs={"Original nollie diagram"};update_trickpedia(variants,1010);
    require(variants.models.at(3).inputs.front().find("Hold LT for nollie")!=std::string::npos,
        "same native card lost its nollie modifier");
    variants.models.at(3).title="ID_SKATEPEDIA_FS_POPSHUVIT";
    variants.models.at(3).inputs={"Original distinct frontside diagram"};update_trickpedia(variants,1020);
    require(variants.models.at(3).inputs.front().find("Experimental")!=std::string::npos &&
        variants.models.at(3).inputs.front().find("R3")!=std::string::npos &&
        variants.models.at(3).inputs.front().find("Down")!=std::string::npos &&
        variants.models.at(3).inputs.front().find("X")!=std::string::npos &&
        variants.models.at(3).inputs.front().find("not available yet")==std::string::npos,
        "frontside card must show its implemented extended binding and experimental status");
    arcade_mode::on=false;update_trickpedia(variants,1030);
    require(variants.models.at(3).inputs==std::vector<std::string>{"Original distinct frontside diagram"},
        "frontside guidance did not restore the native diagram on OFF");
    for(bool nollie:{false,true}) {
        reset();Context impossible;
        impossible.models.at(3).title=nollie?"ID_SKATEPEDIA_NOLLIE_BACKFOOTIMPOSSIBLE":"ID_SKATEPEDIA_FRONTFOOTIMPOSSIBLE";
        impossible.models.at(3).inputs={"Original alternate Impossible diagram"};
        const auto before=impossible.models;update_trickpedia(impossible,1000);
        const auto& inputs=impossible.models.at(3).inputs.front();
        require(inputs.find("R3")!=std::string::npos && inputs.find("Left")!=std::string::npos &&
            inputs.find("X")!=std::string::npos && inputs.find("Nollie Back Foot Impossible")!=std::string::npos &&
            impossible.models.at(3).title==before.at(3).title,
            "alternate Impossible guidance preserves native title and exact variant controls");
        arcade_mode::on=false;update_trickpedia(impossible,1010);
        require(impossible.models==before,"alternate Impossible OFF restores exact native model");
    }
    reset();Context unsupported;
    unsupported.models.at(3).title="ID_SKATEPEDIA_FS_POPSHUVIT_UNDERFLIP";
    unsupported.models.at(3).inputs={"Original frontside underflip diagram"};
    const auto unsupported_original=unsupported.models;update_trickpedia(unsupported,1000);
    require(unsupported.models.at(3).inputs.front().find("Planned")!=std::string::npos &&
        unsupported.models.at(3).inputs.front().find("not available yet")!=std::string::npos,
        "unsupported frontside underflip card must explain availability instead of borrowing ordinary controls");
    arcade_mode::on=false;update_trickpedia(unsupported,1010);
    require(unsupported.models==unsupported_original,"unsupported guidance did not restore exact native models on OFF");
    reset();Context rewritten;update_trickpedia(rewritten,1000);
    rewritten.models.at(3).inputs={"Native stance changed"};update_trickpedia(rewritten,1010);
    arcade_mode::on=false;update_trickpedia(rewritten,1020);
    require(rewritten.models.at(3).inputs==std::vector<std::string>{"Native stance changed"},"native refresh lost on OFF");
    reset();Context hidden;update_trickpedia(hidden,1000);hidden.models.at(2).focus=-1;update_trickpedia(hidden,1010);
    require(hidden.models.at(2).focus==-1 && hidden.models.at(3)==original.at(3),"closing page rewound focus or retained inputs");
    reset();Context unknown;unknown.models.at(3).title="ID_UNRELATED_OLLIE";const auto before=unknown.models;
    update_trickpedia(unknown,1000);require(unknown.models==before && !unknown.creates,"unrelated title matched a trick");
    for(unsigned failure=1;failure<=3;++failure){
        reset();Context failed;failed.fail_copy=failure;
        try{update_trickpedia(failed,1000);}catch(const std::runtime_error&){}
        require(failed.models==original && !trickpedia_page.manager,"failed publication leaked or changed native fields");
    }
    for(unsigned failure=1;failure<=2;++failure){
        reset();Context failed;failed.fail_create=failure;
        try{update_trickpedia(failed,1000);}catch(const std::runtime_error&){}
        require(failed.models==original && !trickpedia_page.manager,"failed allocation leaked");
    }
    reset();Context expired;update_trickpedia(expired,1000);expired.models.erase(3);update_trickpedia(expired,1010);
    require(expired.models.size()==2 && !trickpedia_page.manager,"expired card retained owned models");
    reset();Context changed;update_trickpedia(changed,1000);changed.manager=11;changed.models=original;
    arcade_mode::on=false;update_trickpedia(changed,1010);
    require(changed.models==original && !trickpedia_page.manager,"old registry accessed after replacement");
    std::cout<<"Native Skatepedia input lease: ownership, selection, OFF, close, expiry and failure checks passed\n";
}
