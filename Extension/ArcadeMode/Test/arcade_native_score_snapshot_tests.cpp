#include "Extension/Skater/native_score_snapshot.h"
#include "Engine/Game/Build/20260929/native_scoring.h"
#include "Engine/Game/Build/20260929/score_hud.h"
#include "Engine/Game/Build/20260929/hud_corner.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>
namespace dingosdk::tricks {
namespace {
using Address=std::uintptr_t;
namespace addr=game::build::v20260929;
namespace menu_data {
struct Value { std::uint64_t handle{}; Address type{}; };
struct Ref { Address record{}; std::uint64_t handle{}; };
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class T>T read(Address a){require(a!=0,"null read");T v{};std::memcpy(&v,reinterpret_cast<const void*>(a),sizeof v);return v;}
std::string string(Address at,std::size_t maximum){const auto p=reinterpret_cast<const char*>(at);const auto n=std::find(p,p+maximum,'\0');require(n!=p+maximum,"unterminated string");return {p,n};}
unsigned kind(Address t){return (read<std::uint16_t>(read<Address>(t)+4)>>5)&31;}
unsigned size(Address t){return kind(t)==3?8u:read<std::uint16_t>(read<Address>(t)+6);}
struct Type {
    std::array<std::byte,64> record{}; Address meta{reinterpret_cast<Address>(record.data())};
    Type(unsigned k,unsigned bytes,std::uint32_t hash,Address element=0){
        const auto flags=static_cast<std::uint16_t>(k<<5),n=static_cast<std::uint16_t>(bytes);
        std::memcpy(record.data(),&hash,4);std::memcpy(record.data()+4,&flags,2);
        std::memcpy(record.data()+6,&n,2);std::memcpy(record.data()+addr::native_scoring::array_element_type,&element,sizeof element);
    }
    Address address()const{return reinterpret_cast<Address>(&meta);}
};
struct Node {
    Address type{};std::vector<std::byte> bytes;
    std::map<std::uint32_t,Value> fields;std::vector<Value> elements;
    unsigned count{},stride{};
};
struct Context {
    Address manager{1};std::vector<std::unique_ptr<Type>> types;
    mutable std::map<std::uint64_t,Node> nodes;
    mutable unsigned reference_reads{};bool replace_reference{},expire_owner{};
    std::uint64_t reference_handle{},scoring_handle{};
    Address type(unsigned k,unsigned n,std::uint32_t hash=0,Address element=0){
        types.push_back(std::make_unique<Type>(k,n,hash,element));return types.back()->address();
    }
    Value make(Address t){const auto h=nodes.size()+1;nodes.emplace(h,Node{t,std::vector<std::byte>(size(t)),{},{},0,0});return {h,t};}
    template<class T>Value scalar(Address t,const T& value){auto v=make(t);put(v,value);return v;}
    template<class T>void put(Value v,const T& value)const{auto& b=nodes.at(v.handle).bytes;require(b.size()==sizeof value,"fixture size");std::memcpy(b.data(),&value,sizeof value);}
    Value field(Value v,std::uint32_t h)const{return nodes.at(v.handle).fields.at(h);}
    Address address(Value v)const{
        if(v.handle==reference_handle && ++reference_reads==2 && replace_reference){auto old=read<Ref>(reinterpret_cast<Address>(nodes.at(v.handle).bytes.data()));++old.handle;put(v,old);}
        return reinterpret_cast<Address>(nodes.at(v.handle).bytes.data());
    }
    Address type_of(std::uint64_t h)const{if(expire_owner && h==scoring_handle)return 0;auto it=nodes.find(h);return it==nodes.end()?0:it->second.type;}
    std::vector<std::byte> array(Value v,unsigned maximum,unsigned& count,unsigned& stride)const{
        const auto& n=nodes.at(v.handle);count=n.count;stride=n.stride;require(count<=maximum,"array limit");return n.bytes;
    }
    Value element(Value v,unsigned i)const{return nodes.at(v.handle).elements.at(i);}
};
}
#include "Extension/Skater/native_score_snapshot.inl"
namespace layout=addr::native_scoring;
struct Fixture {
    menu_data::Context c;
    Address u32=c.type(16,4),enum_type=c.type(8,4),action=c.type(3,112,layout::action_schema);
    Address sequence_type=c.type(2,layout::sequence_size,layout::sequence_schema),line_type=c.type(2,layout::line_size,layout::line_schema);
    Address action_array=c.type(4,8,0,action),sequence_array=c.type(4,8,0,sequence_type);
    menu_data::Value hud=c.make(c.type(2,addr::score_hud::score_view_size,addr::hud_corner::score_view_schema));
    menu_data::Value scoring=c.make(c.type(2,layout::size,layout::schema));
    menu_data::Value reference=c.scalar(c.type(2,16),menu_data::Ref{42,scoring.handle});
    std::array<std::byte,24> name_field{};
    std::map<std::uint64_t,std::array<std::byte,layout::action_size>> action_storage;
    std::uint64_t object(std::uint64_t id) {
        if (!id) return 0;
        auto& bytes=action_storage[id];
        const char* name=id==101?"ollie":"heelflip";
        std::memcpy(bytes.data()+96,&name,sizeof name);
        return reinterpret_cast<std::uint64_t>(bytes.data());
    }
    menu_data::Value seq(std::uint32_t points,std::initializer_list<std::uint64_t> objects){
        auto v=c.make(sequence_type);auto& f=c.nodes.at(v.handle).fields;
        f[layout::sequence_points]=c.scalar(u32,points);f[layout::sequence_index]=c.scalar(u32,std::uint32_t{0});
        f[layout::sequence_kind]=c.scalar(enum_type,std::uint32_t{1});
        f[layout::current_action]=c.scalar(action,objects.size()?*objects.begin():std::uint64_t{0});
        auto a=c.make(action_array);auto& n=c.nodes.at(a.handle);n.count=static_cast<unsigned>(objects.size());n.stride=8;n.bytes.resize(objects.size()*8);
        std::vector<std::uint64_t> pointers;for(const auto id:objects)pointers.push_back(object(id));
        if(!pointers.empty())std::memcpy(n.bytes.data(),pointers.data(),n.bytes.size());f[layout::sequence_actions]=a;return v;
    }
    menu_data::Value line(std::initializer_list<menu_data::Value> sequences){
        auto v=c.make(line_type),a=c.make(sequence_array);auto& n=c.nodes.at(a.handle);n.count=static_cast<unsigned>(sequences.size());n.stride=layout::sequence_size;
        n.bytes.resize(n.count*n.stride);n.elements=sequences;c.nodes.at(v.handle).fields[layout::line_sequences]=a;return v;
    }
    Fixture(){
        const auto text=c.type(7,8),fields=reinterpret_cast<Address>(name_field.data());
        const auto meta=menu_data::read<Address>(action);
        const std::uint16_t count=1,offset=96;
        std::memcpy(reinterpret_cast<void*>(meta+layout::field_count),&count,2);
        std::memcpy(reinterpret_cast<void*>(meta+layout::class_fields),&fields,8);
        std::memcpy(name_field.data(),&layout::action_name,4);
        std::memcpy(name_field.data()+layout::field_offset,&offset,2);
        std::memcpy(name_field.data()+layout::field_type,&text,8);
        c.reference_handle=reference.handle;c.scoring_handle=scoring.handle;c.nodes.at(hud.handle).fields[addr::score_hud::scoring_data]=reference;
        auto& f=c.nodes.at(scoring.handle).fields;f[layout::current_sequence]=seq(15,{101});f[layout::previous_sequence]=seq(13,{91});
        f[layout::current_line]=line({f.at(layout::previous_sequence)});f[layout::previous_line]=line({});
    }
    auto sample(){return native_scoring_read::snapshot(c,hud);}
    auto current(){return c.field(scoring,layout::current_sequence);}
};
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class Change>void rejects(Change change,const char* why){Fixture f;change(f);bool refused=false;try{(void)f.sample();}catch(...){refused=true;}check(refused,why);}
}
int test_snapshot(){
    Fixture f;auto a=f.sample();check(a && a->current.points==15 && a->current.action_objects==std::vector<std::uint64_t>{f.object(101)} && a->current.ollie_actions==a->current.action_objects && a->previous.ollie_actions.empty() &&
        a->completed.size()==1 && a->completed[0].points==13 && a->previous_line.empty(),"native sequences and raw field values read independently of labels");
    const auto held=f.sample();check(held && held->current.action_objects==a->current.action_objects,"held frame retains native action identity");
    Fixture absent;absent.c.put(absent.reference,menu_data::Ref{});check(!absent.sample(),"empty scoring ref is unavailable, not a zero award");
    rejects([](auto& x){x.c.put(x.reference,menu_data::Ref{0,x.scoring.handle});},"incomplete reference accepted");
    rejects([](auto& x){x.c.expire_owner=true;},"expired scoring model accepted");
    rejects([](auto& x){x.c.replace_reference=true;},"changed scoring owner accepted");
    rejects([](auto& x){auto v=x.c.field(x.current(),layout::sequence_actions);x.c.nodes.at(v.handle).count=65;},"oversized action list accepted");
    rejects([](auto& x){auto v=x.c.field(x.current(),layout::sequence_actions);x.c.nodes.at(v.handle).stride=4;},"wrong action stride accepted");
    rejects([](auto& x){auto v=x.c.field(x.current(),layout::sequence_actions);x.c.nodes.at(v.handle).bytes.clear();},"short action snapshot accepted");
    rejects([](auto& x){auto v=x.c.field(x.current(),layout::sequence_actions);x.c.put(v,std::uint64_t{0});},"null action accepted");
    rejects([](auto& x){auto& fields=x.c.nodes.at(x.current().handle).fields;fields.erase(layout::sequence_points);},"missing native point field accepted");
    rejects([](auto& x){auto& v=x.c.nodes.at(x.current().handle).fields.at(layout::sequence_points);v.type=x.enum_type;},"wrong native score type accepted");
    rejects([](auto& x){x.name_field.fill(std::byte{});},"missing action name declaration accepted");
    rejects([](auto& x){const std::uint16_t bad=110;std::memcpy(x.name_field.data()+layout::field_offset,&bad,2);},"out-of-bounds action name accepted");
    rejects([](auto& x){auto& object=x.action_storage.at(101);std::memset(object.data()+96,0,8);},"missing action name accepted");
    std::cout<<"Native scoring production reader: schemas, ownership, held records and failure checks passed\n";return 0;
}
}
int main(){return dingosdk::tricks::test_snapshot();}
