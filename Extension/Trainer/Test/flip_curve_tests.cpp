#include "Extension/Trainer/trainer_flip_curve.h"
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <vector>
using namespace dingosdk::trainer::flip_curve_detail;
namespace {
unsigned checks{};
void check(bool value,const char* why){++checks;if(!value){std::cerr<<why<<'\n';std::exit(1);}}
struct Fixture {
    static constexpr std::uintptr_t object=0x10000, data=0x20000, type=0x30000;
    std::map<std::uintptr_t,std::vector<unsigned char>> memory;
    std::set<std::uintptr_t> unreadable,unwritable;
    std::function<void(std::uintptr_t)> before_write;
    unsigned writes{};Curve curve{};
    template<class T> void seed(std::uintptr_t at,T value){auto& bytes=memory[at];bytes.resize(sizeof value);std::memcpy(bytes.data(),&value,sizeof value);}
    template<class T> bool read(std::uintptr_t at,T& value){
        const auto i=memory.find(at);if(unreadable.count(at)||i==memory.end()||i->second.size()!=sizeof value)return false;
        std::memcpy(&value,i->second.data(),sizeof value);return true;
    }
    bool write(std::uintptr_t at,float expected,float value){
        if(before_write)before_write(at);
        float current{};if(unwritable.count(at)||!read(at,current)||!equal(current,expected))return false;
        seed(at,value);++writes;return true;
    }
    float get(std::uintptr_t at){float value{};check(read(at,value),"fixture read is available");return value;}
    Fixture(unsigned count=3,float low=.4f,float high=1.4f){
        seed(object,type);seed(object+points_at,data);seed(data-4,static_cast<std::uint32_t>(count));
        seed(object+0x20,low);seed(object+0x24,high);
        for(unsigned i=0;i<count;++i){seed(data+i*point_stride+point_x,static_cast<float>(i)*.45f);seed(data+i*point_stride+point_y,i==0?.4f:i==1?1.f:1.4f);}
        check(bind(count),"fixture captures an existing resolved two/three-point curve");
    }
    bool bind(unsigned count=3){auto reader=[&](std::uintptr_t at,auto& value){return read(at,value);};return capture(reader,object,type,count,0,.4f,curve);}
    Result step(float factor){
        auto reader=[&](std::uintptr_t at,auto& value){return read(at,value);};
        auto writer=[&](std::uintptr_t at,float expected,float value){return write(at,expected,value);};
        return apply(reader,writer,curve,factor);
    }
};
void scaling(){
    {
        Fixture f;
        for(unsigned i=0;i<3;++i)f.seed(Fixture::data+i*point_stride+point_y,0.f);
        auto reader=[&](std::uintptr_t at,auto& value){return f.read(at,value);};
        check(capture(reader,Fixture::object,Fixture::type,3,0,0,f.curve),"zero-output curve retains its positive clamp bounds");
        const auto result=f.step(2);
        check(result.status==Status::applied&&result.outputs_written==0&&f.writes==2,"clamp-only rate changes report applied without inflating output count");
    }
    for(unsigned count:{2u,3u})for(float low:{-.4f,0.f,.4f}){
        Fixture f(count,low);const auto baseline=f.memory;
        auto result=f.step(2);check(result.status==Status::applied&&result.outputs_written==count,"known rate change applies the captured point count");
        check(f.get(Fixture::data+point_y)==.8f&&f.get(Fixture::data+point_stride+point_y)==2.f,"outputs scale from the captured baseline");
        check(f.get(Fixture::object+0x20)==(low>0?low*2:low)&&f.get(Fixture::object+0x24)==2.8f,"positive clamp bounds follow the multiplier and nonpositive bounds retain source policy");
        if(count==3)check(f.get(Fixture::data+2*point_stride+point_y)==2.8f,"third point uses the same selected factor");
        const auto writes=f.writes;check(f.step(2).status==Status::unchanged&&f.writes==writes,"repeated same factor has no writes");
        check(f.step(.5f).status==Status::applied&&f.get(Fixture::data+point_y)==.2f,"factor changes never multiply an already scaled curve");
        check(f.step(1).status==Status::applied&&f.memory==baseline,"owned reset restores exact original bytes including bounds and inputs");
    }
}
void external_changes(){
    for(bool scaled:{false,true})for(unsigned field=0;field<5;++field){
        Fixture f;if(scaled)f.step(2);f.writes=0;
        f.seed(address(f.curve,field),9.f);const auto before=f.memory;
        check(f.step(scaled?1:2).status==Status::retired&&f.memory==before&&!f.writes,
              "any current output or clamp overwrite withdraws the whole curve before change or restore");
    }
    for(unsigned fault=0;fault<8;++fault){
        Fixture f;
        if(fault==0)f.seed(Fixture::object,Fixture::type+8);
        if(fault==1)f.seed(Fixture::object+points_at,Fixture::data+0x1000);
        if(fault==2)f.seed(Fixture::data-4,std::uint32_t{2});
        if(fault==3)f.seed(Fixture::data-4,std::uint32_t{0x80000003u});
        if(fault==4)f.seed(Fixture::data+point_x,.001f);
        if(fault==5)f.seed(Fixture::data+point_stride+point_x,.5f);
        if(fault==6)f.seed(Fixture::data+point_y,std::numeric_limits<float>::quiet_NaN());
        if(fault==7)f.seed(Fixture::object+0x24,std::numeric_limits<float>::infinity());
        const auto before=f.memory;
        check(f.step(2).status==Status::retired&&f.memory==before&&!f.writes,"replaced identity/count/input or nonfinite current data never authorizes writes");
    }
    Fixture f;const auto before=f.memory;f.unreadable.insert(Fixture::data+point_y);
    check(f.step(2).status==Status::unavailable&&f.memory==before&&!f.writes,"unreadable current state is unavailable without mutation");
    f.unreadable.clear();check(f.step(2).status==Status::applied,"transient unreadability does not discard a still-identical baseline");
}
void malformed(){
    for(unsigned field=0;field<5;++field)for(float invalid:{std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        Fixture f;f.seed(address(f.curve,field),invalid);check(!f.bind()&&!f.curve.valid,"all captured outputs/bounds must be finite");
    }
    for(unsigned field=0;field<3;++field){Fixture f;f.seed(Fixture::data+field*point_stride+point_x,std::numeric_limits<float>::quiet_NaN());check(!f.bind(),"every captured input coordinate must be finite");}
    for(float factor:{0.f,-1.f,.01f,100.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
        Fixture f;const auto before=f.memory;check(f.step(factor).status==Status::unavailable&&f.memory==before&&!f.writes,"invalid requested factors cannot write");
    }
    Fixture overflow;overflow.seed(Fixture::data+point_stride+point_y,std::numeric_limits<float>::max());check(overflow.bind(),"finite baseline can be represented without multiplying yet");
    const auto before=overflow.memory;check(overflow.step(2).status==Status::unavailable&&overflow.memory==before&&!overflow.writes,"nonfinite planned result fails before the first write");
    Fixture f;auto reader=[&](std::uintptr_t at,auto& value){return f.read(at,value);};Curve out{};
    for(unsigned count:{0u,1u,4u,1000u})check(!capture(reader,Fixture::object,Fixture::type,count,0,.4f,out),"unsupported point counts do not expand the established surface");
    check(!capture(reader,Fixture::object,Fixture::type+8,3,0,.4f,out),"capture requires the worker's exact resolved type");
}
void failed_writes(){
    for(unsigned field=0;field<5;++field){
        Fixture f;const auto baseline=f.memory;f.unwritable.insert(address(f.curve,field));
        check(f.step(2).status==Status::retired&&f.memory==baseline&&!f.curve.pending,"partial update rolls back only its own writes before retiring");
    }
    for(unsigned blocked=1;blocked<5;++blocked){
        Fixture f;const auto baseline=f.memory;const auto first=address(f.curve,0),last=address(f.curve,blocked);
        f.before_write=[&](std::uintptr_t at){if(at==last){f.unwritable.insert(last);f.unwritable.insert(first);}};
        check(f.step(2).status==Status::restore_pending&&f.curve.pending&&needs_work(f.curve,2),"failed rollback retains bounded per-cell ownership after requested setting was processed");
        f.before_write={};f.unwritable.clear();
        check(f.step(2).status==Status::retired&&f.memory==baseline&&!f.curve.pending,"later cleanup retries the exact owned writes before retiring, without reapplying the request");
    }
    {
        Fixture f;const auto first=address(f.curve,0),second=address(f.curve,1);
        f.before_write=[&](std::uintptr_t at){if(at==second){f.seed(first,9.f);f.unwritable.insert(second);}};
        check(f.step(2).status==Status::retired&&f.get(first)==9.f&&!f.curve.pending,"rollback never restores a field overwritten after our partial write");
    }
    {
        Fixture f;const auto second=address(f.curve,1);
        f.before_write=[&](std::uintptr_t at){if(at==second)f.seed(second,9.f);};
        check(f.step(2).status==Status::retired&&f.get(second)==9.f&&f.get(address(f.curve,0))==.4f,"compare-and-write preserves a native overwrite in the write window and restores only the preceding owned field");
    }
    {
        Fixture f;const auto first=address(f.curve,0),second=address(f.curve,1);
        f.before_write=[&](std::uintptr_t at){if(at==second){f.seed(Fixture::object+points_at,Fixture::data+0x1000);f.unwritable.insert(second);}};
        check(f.step(2).status==Status::retired&&f.get(first)==.8f&&!f.curve.pending,"replacement during a partial update forbids restoring the former allocation");
    }
    {
        Fixture f;const auto original=f.memory;const auto first=address(f.curve,0),second=address(f.curve,1);
        f.before_write=[&](std::uintptr_t at){if(at==second){f.unwritable.insert(second);f.unreadable.insert(first);}};
        check(f.step(2).status==Status::restore_pending&&f.curve.pending,"unreadable owned prefix retains its cleanup obligation");
        f.before_write={};f.unwritable.clear();
        check(f.step(3).status==Status::restore_pending&&f.curve.pending,"a new desired factor does not erase unreadable pending cleanup");
        f.unreadable.clear();check(f.step(3).status==Status::retired&&f.memory==original,"cleanup retries exact prior bytes once the owned field is readable again");
    }
}
}
int main(){scaling();external_changes();malformed();failed_writes();std::cout<<checks<<" resolved flip-curve ownership checks passed\n";}
