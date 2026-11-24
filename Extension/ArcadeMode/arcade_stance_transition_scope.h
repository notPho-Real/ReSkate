#pragma once
#include "Engine/Game/Build/20260929/arcade_stance.h"

namespace dingosdk::arcade_mode::native_stance {
// A generic EAT callback also serves NPCs and unrelated actions. Authenticate
// the original expression, its actual call arguments and the registered local
// entity before supplying an Arcade RT request to that callback.
template<class Read>
bool transition_scope(Read&& read,std::uintptr_t image,std::uintptr_t vm,
        std::uintptr_t context,std::uintptr_t predicate,std::uintptr_t flags,
        std::uintptr_t entity,std::uintptr_t asset) noexcept {
    namespace c=game::build::v20260929::arcade_stance;
    namespace e=game::build::v20260929::arcade_fingerflip;
    std::uintptr_t instance{},resource{},instance_resource{};
    std::uint32_t key{};std::array<std::uint32_t,10> layout{};
    if(!image || !vm || !entity || !context || !predicate || !flags ||
       !read(vm+e::vm_instance,instance) || !instance ||
       !read(vm+e::vm_resource,resource) || !resource ||
       !read(instance,instance_resource) || resource!=instance_resource ||
       !read(resource+e::resource_key,key) || key!=c::regular_transition_key ||
       !read(resource+e::resource_layout,layout) || layout!=c::regular_transition_layout)return false;
    c::Guid guid{};std::uintptr_t graph{},wrapper{},asset_resource{};
    if(asset<0x10000+c::asset_guid_prefix || !read(asset-c::asset_guid_prefix,guid) ||
       guid!=c::transition_asset.guid || !read(asset+c::expression_graph,graph) || !graph ||
       !read(graph+c::graph_resource_wrapper,wrapper) || !wrapper ||
       !read(wrapper+c::wrapper_resource,asset_resource) || asset_resource!=resource)return false;
    const auto pages=instance+((layout[0]+15u)&~std::uintptr_t{15});
    std::uintptr_t context_page{},predicate_page{},flags_page{};
    if(!read(pages+c::transition_context_page,context_page) || context_page!=context ||
       !read(pages+c::transition_predicate_page,predicate_page) || !predicate_page ||
       predicate_page+c::transition_predicate_offset!=predicate ||
       !read(pages+c::transition_flags_page,flags_page) || !flags_page ||
       flags_page+c::transition_flags_offset!=flags)return false;
    std::uintptr_t arguments{},reference{},context_key{},registry{},entries{},registered{},registered_entity{};
    std::uint32_t mask{};
    if(!read(pages+e::argument_page,arguments) || !arguments ||
       !read(arguments+e::context_argument,reference) || !reference ||
       !read(reference,context_key) || !context_key ||
       !read(image+e::context_registry,registry) || !registry ||
       !read(registry+e::registry_entries,entries) || !entries ||
       !read(registry+e::registry_mask,mask) || mask>0xffffff)return false;
    const auto entry=entries+(context_key&mask)*e::registry_entry_size;
    return read(entry,registered) && registered==context_key &&
        read(entry+e::registry_entity,registered_entity) && registered_entity==entity;
}
}
