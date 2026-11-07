#pragma once
#include "Engine/Game/Build/20260929/arcade_fingerflip.h"

namespace dingosdk::arcade_mode::native_grab {
// Restrict the borrowed preference to the original EAT conditions for this
// local skater. UI readers, NPCs and other expressions keep their own values.
template<class Read>
bool finger_setting_scope(Read&& read,std::uintptr_t image,std::uintptr_t vm,
                          std::uintptr_t entity) noexcept {
    namespace c=game::build::v20260929::arcade_fingerflip;
    std::uintptr_t instance{},resource{},instance_resource{};
    std::uint32_t key{};std::array<std::uint32_t,10> layout{};
    if(!image || !vm || !entity || !read(vm+c::vm_instance,instance) || !instance ||
       !read(vm+c::vm_resource,resource) || !resource || !read(instance,instance_resource) ||
       resource!=instance_resource || !read(resource+c::resource_key,key) ||
       !read(resource+c::resource_layout,layout))return false;
    bool exact{};
    for(const auto& expected:c::setting_expressions)
        if(key==expected.key && layout==expected.layout)exact=true;
    if(!exact)return false;
    // ContextKey is the second reference argument. Read the original argument
    // page: these conditions ask for the option before binding page3 locally.
    const auto pages=instance+((layout[0]+15u)&~std::uintptr_t{15});
    std::uintptr_t arguments{},reference{},context_key{},registry{},entries{},registered{},owner{};
    std::uint32_t mask{};
    if(!read(pages+c::argument_page,arguments) || !arguments ||
       !read(arguments+c::context_argument,reference) || !reference ||
       !read(reference,context_key) || !context_key ||
       !read(image+c::context_registry,registry) || !registry ||
       !read(registry+c::registry_entries,entries) || !entries ||
       !read(registry+c::registry_mask,mask) || mask>0xffffff)return false;
    const auto entry=entries+(context_key&mask)*c::registry_entry_size;
    return read(entry,registered) && registered==context_key &&
        read(entry+c::registry_entity,owner) && owner==entity;
}
}
