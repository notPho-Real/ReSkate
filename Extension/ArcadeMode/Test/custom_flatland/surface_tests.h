#pragma once
#include "Extension/ArcadeMode/arcade_flatland_surface.h"
#include <cstring>
#include <map>
#include <iostream>
#include <cstdlib>
namespace {
void surface_tests() {
    using namespace dingosdk;
    using namespace arcade_mode;
    namespace c=game::build::v20260929::contact_material;
    namespace v=game::build::v20260929::native_vfx;
    unsigned count=0;
    const auto require=[&](bool ok,const char* why){
        ++count;if(!ok){std::cerr<<"FAIL surface: "<<why<<'\n';std::exit(1);}
    };
    struct Memory {
        std::map<std::uintptr_t,unsigned char> bytes;
    };
    Memory memory;
    const auto put=[&](std::uintptr_t at,const auto& value) {
        const auto* raw=reinterpret_cast<const unsigned char*>(&value);
        for(std::size_t i=0;i<sizeof(value);++i)memory.bytes[at+i]=raw[i];
    };
    const auto get=[&](std::uintptr_t at,auto& value) {
        auto* raw=reinterpret_cast<unsigned char*>(&value);
        for(std::size_t i=0;i<sizeof(value);++i){
            const auto it=memory.bytes.find(at+i);if(it==memory.bytes.end())return false;raw[i]=it->second;
        }
        return true;
    };
    constexpr std::uintptr_t base=0x10000000,world=0x20000000,grid=0x30000000,data=0x40000000,
        index=0x50000000,properties=0x60000000,list=0x70000000,reference=0x71000000,
        object=0x72000000,enumeration=0x73000000,name=0x74000000;
    put(base+v::materials_offset,std::uint32_t{0x1000});
    put(world+0x1000+v::materials_ready,std::uint8_t{1});
    put(world+0x1000+v::materials_grid,grid);
    put(grid+v::grid_data,data|std::uintptr_t{4});
    put(data+v::grid_index_table,index);put(index-4,std::uint32_t{2});
    put(index,std::uint32_t{1});put(index+4,std::uint32_t{0});
    put(data+c::properties,properties);put(properties-4,std::uint32_t{2});
    put(properties+c::row_size,list);put(list-4,std::uint32_t{1});
    put(list,reference);put(reference,object|std::uintptr_t{4});
    put(object+c::object_type,base+c::gameplay_type);
    put(object+c::material_enum,enumeration);put(enumeration+c::object_type,base+c::enum_type);
    put(enumeration+c::enum_name,name);put(enumeration+c::enum_value,std::uint32_t{11});
    const char metal[]="Metal";for(std::size_t i=0;i<sizeof(metal);++i)put(name+i,metal[i]);
    const auto original=memory;
    const auto read=[&](std::uint32_t packed=32){return contact_material_read::read(base,world,packed,get);};
    auto result=read();
    require(result && result->row==1 && result->label()=="Metal" && result->value==11,"slot is remapped through live grid");
    require(!read(0),"invalid packed material");
    require(!read(32|(2<<6)),"slot bounds");
    put(index,std::uint32_t{2});require(!read(),"row bounds");memory=original;
    put(list-4,std::uint32_t{33});require(!read(),"property bounds");memory=original;
    put(object+c::object_type,std::uintptr_t{99});require(!read(),"unknown property type");memory=original;
    put(object+c::material_enum,std::uintptr_t{});require(!read(),"absent enum");memory=original;
    put(enumeration+c::object_type,std::uintptr_t{99});require(!read(),"wrong enum type");memory=original;
    put(list-4,std::uint32_t{2});put(list+8,reference);require(!read(),"duplicate gameplay property");memory=original;
    for(std::size_t i=0;i<64;++i)put(name+i,'A');
    require(!read(),"unterminated name");memory=original;
    put(name,char{1});require(!read(),"nonprintable name");memory=original;
    unsigned visits=0;
    auto changed=[&](std::uintptr_t at,auto& value){
        if(at==world+0x1000+v::materials_grid && ++visits==2)put(index,std::uint32_t{0});
        return get(at,value);
    };
    require(!contact_material_read::read(base,world,32,changed),"changed grid mapping refused");memory=original;
    require(flatland_surface_kind("Default")==FlatlandSurface::ordinary,"default ground");
    require(flatland_surface_kind("Asphalt")==FlatlandSurface::rough,"rough ground");
    require(flatland_surface_kind("Wood_Thick_Smooth")==FlatlandSurface::sealed,"sealed trick surface");
    require(flatland_surface_kind("Metal")==FlatlandSurface::slick,"slick trick surface");
    require(flatland_surface_kind("Unknown")==FlatlandSurface::unknown,"unknown name");
    NativeInputFrame frame{};frame.valid=true;frame.owner={1,2,3,4,5};
    frame.owner_generation=1;frame.contact_generation=1;frame.physics_step=1;
    frame.sampled_at=1000;frame.physics_seconds=1.f/60;
    native_wall_contact::SupportMaterial support{};
    support.valid=true;support.owner={1,2,3,4,5};support.serial=1;support.sampled_at=1000;
    support.seconds=frame.physics_seconds;support.material=*read();
    FlatlandSurfaceJoin join;
    require(!join.observe(frame,support).verified_support_material,"first sample cannot prove interval");
    auto next=[&]{++frame.physics_step;frame.sampled_at+=17;++support.serial;support.sampled_at=frame.sampled_at;};
    next();auto selected=join.observe(frame,support);
    require(selected.verified_support_material && selected.surface==FlatlandSurface::slick,"same original step selects contact material");
    require(std::abs(flatland_friction(frame,selected).rolling*60*GameplayProfile::metres_per_inch-.75f)<.00001f,"selected material reaches actual resistance policy");
    require(!join.observe(frame,support).verified_support_material,"duplicate step");
    next();--support.serial;require(!join.observe(frame,support).verified_support_material,"reused contact publication");
    next();support.sampled_at-=34;require(!join.observe(frame,support).verified_support_material,"stale contact");
    next();++support.sampled_at;require(!join.observe(frame,support).verified_support_material,"future contact");
    next();support.seconds=.02f;require(!join.observe(frame,support).verified_support_material,"different native time step");
    support.seconds=frame.physics_seconds;
    next();++support.owner.rig;require(!join.observe(frame,support).verified_support_material,"different owner");
    --support.owner.rig;
    next();++frame.owner_generation;require(!join.observe(frame,support).verified_support_material,"reused owner address");
    next();support.valid=false;require(!join.observe(frame,support).verified_support_material,"unqualified support");
    support.valid=true;
    next();support.material.name={};require(!join.observe(frame,support).verified_support_material,"unknown support defaults");
    std::cout<<count<<" live material reader and step join checks passed (synthetic memory)\n";
}
}
