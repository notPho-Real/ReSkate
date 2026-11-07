#pragma once
#include "Engine/Game/Build/20260929/arcade_wall_contact.h"

namespace dingosdk::arcade_mode::contact_owner {
// fee430 scans this original provider table by exact physics owner. Unlike
// its native fallback, an absent/duplicate owner never borrows the first row.
template<class Read>
std::uintptr_t vector(std::uintptr_t provider,std::uintptr_t owner,Read&& read) noexcept {
    namespace c=game::build::v20260929::arcade_wall_contact;
    if(!provider||!owner)return 0;
    std::array<std::uintptr_t,2> range{},after{};
    if(!read(provider+c::provider_owners,range)||!range[0]||range[1]<range[0]||
       (range[1]-range[0])%c::provider_owner_size||
       (range[1]-range[0])/c::provider_owner_size>128)return 0;
    std::uintptr_t found{};
    for(auto at=range[0];at<range[1];at+=c::provider_owner_size) {
        std::uintptr_t key{};
        if(!read(at,key))return 0;
        if(key==owner) {if(found)return 0;found=at;}
    }
    std::uintptr_t key{};
    if(!found||!read(provider+c::provider_owners,after)||range!=after||!read(found,key)||key!=owner)return 0;
    return found+c::provider_contacts;
}
}
