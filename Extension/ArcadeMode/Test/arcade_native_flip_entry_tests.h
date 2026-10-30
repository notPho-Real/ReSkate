#pragma once
#include "Extension/ArcadeMode/arcade_native_flip_entry.h"
#include <cstring>
#include <vector>
#include <iostream>
#include <cstdlib>
namespace native_entry_tests {
using namespace dingosdk::arcade_mode::native_flip;
struct Memory {
    std::vector<unsigned char> bytes=std::vector<unsigned char>(0x3000);
    static constexpr std::uintptr_t base=0x140000000ULL;
    template<class T> void put(std::uintptr_t at,T value) { std::memcpy(bytes.data()+at-0x10000,&value,sizeof(value)); }
    template<class T> bool read(std::uintptr_t at,T& value) {
        if(at<0x10000 || at-0x10000+sizeof(value)>bytes.size())return false;
        std::memcpy(&value,bytes.data()+at-0x10000,sizeof(value));return true;
    }
    void ptr(std::uintptr_t at,std::uintptr_t value) { put(at,value); }
    explicit Memory(bool four_byte_arrays=false) {
        // Scalar layout and tag timing independently captured from native
        // Kickflip high. No skeletons, animations or other game assets included.
        put(0x10000,std::uint32_t{0x8400000d});ptr(0x10008,0x10100);ptr(0x101d0,0x10400);
        ptr(0x10430,0x10500);ptr(0x10500,base+0x61620e8);ptr(0x10528,0x10600);ptr(0x10538,0x11100);
        ptr(0x10600,base+0x61bab98);ptr(0x10640,0x10700);put(0x106fc,1u);ptr(0x10700,0x10800);
        ptr(0x10800,base+0x61ba838);ptr(0x10830,0x10900);ptr(0x10928,0x10a00);put(0x109fc,1u);
        ptr(0x10a00,0x10b00);ptr(0x10b28,0x10c00);put(0x10bfc,2u);ptr(0x10c00,0x10d00);ptr(0x10c08,0x10e00);
        for(auto tag:{0x10d00u,0x10e00u}) {
            ptr(tag,base+0x6189e38);put(tag+0x28,12.f);put(tag+0x30,2.f);put(tag+0x50,1u);
        }
        ptr(0x10d40,0x10f10);ptr(0x10e40,0x11010);put(0x10f00,pop_guid);put(0x11000,jump_guid);
        put(0x110fc,20u);for(unsigned i=0;i<20;++i)put(0x11104+16*i,float(i*2));
        if(four_byte_arrays) {
            auto shift=[&](std::uintptr_t at,std::uintptr_t data,unsigned count,unsigned stride) {
                std::memmove(bytes.data()+data-0x10000+4,bytes.data()+data-0x10000,count*stride);
                put(data,count);ptr(at,data+4);
            };
            shift(0x10640,0x10700,1,8);shift(0x10928,0x10a00,1,8);
            shift(0x10b28,0x10c00,2,8);shift(0x10538,0x11100,20,16);
        }
    }
    auto entry() { return post_pop_entry([&](auto at,auto& value){return read(at,value);},base,0x10000); }
};
inline void check(bool ok,const char* why) { if(!ok){std::cerr<<"FAIL native entry: "<<why<<'\n';std::exit(1);} }
inline void run() {
    // Native arrays can retain address bit 2; only object references tag it.
    for(bool four_byte_arrays:{false,true}) for(unsigned initial:{0u,3u,5u,6u,7u}) {
        Memory m(four_byte_arrays);m.put(0x10000,0x8400000du|(initial<<12));const auto out=m.entry();
        check(out && out->tick==16 && out->pop_end==14,"first sample strictly beyond the inclusive native pop interval");
        check(out->key==0x8400800d && out->clip==0x10500,"native sequence, database index, mirror and other key bits preserved");
    }
    Memory later;later.put(0x10000,0x8400800du);check(!later.entry(),"already post-pop selection is unchanged");
    for(unsigned fault=0;fault<7;++fault) {
        Memory m;
        if(fault==0)m.put(0x10000,0xffffffffu);
        if(fault==1)m.ptr(0x10500,0);
        if(fault==2)m.put(0x110fc,0x7fffffffu);
        if(fault==3)m.put(0x10d2c,std::uint16_t{0x40});
        if(fault==4)m.ptr(0x10d38,0x12345);
        if(fault==5)m.put(0x11000,Guid{});
        if(fault==6)m.put(0x10d50,0u);
        check(!m.entry(),"unknown layout, conditional/scaled/unbound pop or missing jump fails closed");
    }
    std::cout<<"Native motion-entry boundary and preservation checks passed\n";
}
}
