#pragma once
#include "Engine/Game/Build/20260929/contact_material.h"
#include "Engine/Game/Build/20260929/native_vfx.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
namespace dingosdk::contact_material_read {
struct Material {
    std::uint32_t packed{},slot{},row{},value{};
    std::array<char,64> name{};
    std::array<std::uintptr_t,9> identity{};
    bool operator==(const Material&) const = default;
    std::string_view label() const noexcept {
        for(std::size_t i=0;i<name.size();++i)if(!name[i])return {name.data(),i};
        return {};
    }
};
// Only copies borrowed data. The caller supplies the current world and a
// fingerprint-validated image, and fences world/contact ownership around it.
template<class Read>
std::optional<Material> read(std::uintptr_t base,std::uintptr_t world,std::uint32_t packed,Read&& get) {
    namespace c=game::build::v20260929::contact_material;
    namespace v=game::build::v20260929::native_vfx;
    const auto inspect=[&]() -> std::optional<Material> {
        if(!base || !world || !(packed&v::material_valid))return {};
        Material out;out.packed=packed;out.slot=(packed>>v::material_shift)&0x1fffu;
        std::uint32_t offset{},slots{},rows{},count{};
        std::uint8_t ready{};
        std::uintptr_t grid{},data{},index{},properties{},list{},reference{},object{},type{},enumeration{},name{};
        if(!get(base+v::materials_offset,offset) || !offset || offset>0x1000000 ||
           !get(world+offset+v::materials_ready,ready) || !ready ||
           !get(world+offset+v::materials_grid,grid) || !grid ||
           !get(grid+v::grid_data,data) || !(data&~std::uintptr_t{4}))return {};
        data&=~std::uintptr_t{4};
        if(!get(data+v::grid_index_table,index) || index<4 ||
           !get(index-4,slots) || (slots&=0x7fffffffu)>8192 || out.slot>=slots ||
           !get(index+out.slot*sizeof(std::uint32_t),out.row) ||
           !get(data+c::properties,properties) || properties<4 ||
           !get(properties-4,rows) || (rows&=0x7fffffffu)>8192 || out.row>=rows ||
           !get(properties+out.row*c::row_size,list) || list<4 ||
           !get(list-4,count) || (count&=0x7fffffffu)>32)return {};
        bool found=false;
        for(std::uint32_t i=0;i<count;++i) {
            std::uintptr_t ref{},obj{},info{};
            if(!get(list+i*sizeof(std::uintptr_t),ref) || !ref || !get(ref,obj) || !(obj&~std::uintptr_t{4}))return {};
            obj&=~std::uintptr_t{4};
            if(!get(obj+c::object_type,info))return {};
            if(info!=base+c::gameplay_type)continue;
            if(found)return {}; // Ambiguous properties cannot select a policy.
            found=true;reference=ref;object=obj;
        }
        if(!found || !get(object+c::material_enum,enumeration) || !enumeration ||
           !get(enumeration+c::object_type,type) || type!=base+c::enum_type ||
           !get(enumeration+c::enum_name,name) || !name ||
           !get(enumeration+c::enum_value,out.value))return {};
        bool terminated=false;
        for(std::size_t i=0;i<out.name.size();++i) {
            if(!get(name+i,out.name[i]))return {};
            const auto ch=static_cast<unsigned char>(out.name[i]);
            if(!ch){terminated=true;break;}
            if(ch<32 || ch>126)return {};
        }
        if(!terminated || out.label().empty())return {};
        out.identity={world,grid,data,index,properties,list,reference,object,enumeration};
        return out;
    };
    const auto before=inspect();
    const auto after=before?inspect():std::optional<Material>{};
    return before && after && *before==*after ? before : std::optional<Material>{};
}
}
