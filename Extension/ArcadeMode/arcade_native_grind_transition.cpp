#include "arcade_native_grind_transition.h"
#include "arcade_grind_transition_lifetime.h"
#include "Engine/Core/Hooks/hooks.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Game/Build/image_identity.h"
#include <atomic>
#include <mutex>
#include <intrin.h>
#include <Windows.h>

namespace dingosdk::arcade_mode::native_grind {
namespace {
namespace c=game::build::v20260929::arcade_grind_transition;
c::Orientation original_orientation{};
std::uintptr_t image{};
std::atomic<bool> ready{},gameplay_allowed{},request_pending{};
std::atomic<std::uint64_t> tick_expires{};
std::mutex mutex;
struct Lease {
    NoBailSkater owner{};
    std::uint64_t serial{},deadline{};
    bool entered{};
};
Lease lease;
template<class T> bool read(std::uintptr_t p,T& v) noexcept { return memory::peek(p,v); }
std::uintptr_t pointer(std::uintptr_t p) noexcept { std::uintptr_t v{};return read(p,v)?v:0; }
Lease snapshot() noexcept { std::lock_guard lock(mutex);return lease; }
bool valid(const Lease& l) noexcept {
    const auto now=GetTickCount64();
    if (!l.serial || !request_pending.load(std::memory_order_acquire) || !gameplay_allowed.load() ||
        now>l.deadline || now>tick_expires.load()) return false;
    NoBailSkater current{};
    if (!no_bail_skater(current) || current!=l.owner || current.base!=image) return false;
    std::lock_guard lock(mutex);
    return lease.serial==l.serial && lease.owner==l.owner;
}
bool contact(const Lease& l) noexcept {
    const auto selector=pointer(l.owner.core+0x440), output=pointer(pointer(selector)+32);
    std::uint8_t physical{},observed{};
    return output && read(l.owner.context+4016,physical) && physical==1 &&
        read(output+400,observed) && observed==1;
}
bool active_actor(const Lease& l,std::uintptr_t actor=0) noexcept {
    if (!actor)actor=pointer(l.owner.core+c::core_actor);
    auto reader=[](auto p,auto& v){return read(p,v);};
    return grind_orientation_scope(reader,{image,l.owner.core,l.owner.context,actor,image+c::orientation_return});
}
bool write_flag(std::uintptr_t address,std::uint8_t value) noexcept {
    __try { *reinterpret_cast<std::uint8_t*>(address)=value;return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
std::uintptr_t orientation(std::uintptr_t actor,std::uintptr_t output,std::uintptr_t input) {
    const auto caller=reinterpret_cast<std::uintptr_t>(_ReturnAddress());
    if (!pending() || caller!=image+c::orientation_return)
        return original_orientation(actor,output,input);
    const auto l=snapshot();
    if (valid(l) && active_actor(l,actor)) {
        std::lock_guard lock(mutex);
        if (lease.serial==l.serial)lease.entered=true;
    }
    return preserve_grind_transition_lifetime({image,l.owner.core,l.owner.context,actor,caller},
        [](auto p,auto& v){return read(p,v);},write_flag,[&]{return valid(l);},
        [&]{return original_orientation(actor,output,input);});
}
bool expression_owner(std::uintptr_t vm,const Lease& l) noexcept {
    const auto resource=pointer(vm+56),instance=pointer(vm+48);
    std::uint32_t key{};std::array<std::uint32_t,10> layout{};
    if (!vm || !resource || !instance || pointer(instance)!=resource ||
        !read(resource+16,key) || !read(resource+32,layout)) return false;
    if (!((key==0x0542dc73 && layout==std::array<std::uint32_t,10>{64,528,109,558,5,0,0,327723,1114112,197632}) ||
        (key==0x3d9f9b54 && layout==std::array<std::uint32_t,10>{176,792,1004,2072,9,0,0,983152,3538944,16845315}))) return false;
    const auto pages=instance+((layout[0]+15)&~std::uintptr_t{15});
    const auto context_key=pointer(pointer(pages+24));
    const auto registry=pointer(image+0x73c5e80),entries=pointer(registry+32);
    std::uint32_t mask{};
    if (!context_key || !entries || !read(registry+16,mask) || mask>0xffffff) return false;
    const auto entry=entries+(context_key&mask)*320;
    return pointer(entry)==context_key && pointer(entry+8)==l.owner.entity;
}
}
void cancel() noexcept {
    request_pending.store(false,std::memory_order_release);
    std::lock_guard lock(mutex);lease={};
    // No global setting/profile edit needs rollback. Subsequent native reads
    // use their original result; native selection and cleanup own actor exit.
}
bool available() noexcept { return ready.load() && gameplay_allowed.load(); }
bool pending() noexcept { return request_pending.load(std::memory_order_acquire); }
void tick(bool gameplay) noexcept {
    gameplay_allowed.store(gameplay);
    tick_expires.store(gameplay?GetTickCount64()+150:0);
    if (!gameplay) { cancel();return; }
    const auto l=snapshot();if(!l.serial)return;
    std::uint32_t state{};
    if (!valid(l) || !read(l.owner.context+c::selected_state,state) ||
        state<400 || state>406 || (l.entered && state!=406)) cancel();
}
bool arm(const NoBailSkater& owner,std::uint64_t serial,std::uint64_t now,std::uint64_t deadline) noexcept {
    const auto current_time=GetTickCount64();
    if (!available() || current_time>tick_expires.load() || !serial || now>current_time || current_time-now>150 ||
        deadline<=current_time || deadline-now>1500 || owner.base!=image) return false;
    NoBailSkater current{};std::uint32_t state{},identity{};
    const Lease candidate{owner,serial,deadline};
    const auto output=pointer(pointer(pointer(owner.core+0x440))+32);
    if (!no_bail_skater(current) || current!=owner || !read(owner.context+c::selected_state,state) ||
        state<400 || state>405 || !contact(candidate) || !output ||
        // All four native 17-value grind blocks are valid origins, including
        // intermediate boardslides. Invalid=0, the later special variants and
        // Exit=77 are outside this initial control contract.
        !read(output+192,identity) || !identity || identity>68) return false;
    std::lock_guard lock(mutex);
    if (lease.serial) return false;
    lease=candidate;request_pending.store(true,std::memory_order_release);return true;
}
bool setting_override(const void* reference,std::uintptr_t expression) noexcept {
    if (!pending())return false;
    const auto l=snapshot();if(!valid(l))return false;
    auto asset=pointer(reinterpret_cast<std::uintptr_t>(reference))&~std::uintptr_t{4};
    constexpr std::array<std::uint8_t,16> expected{0x52,0xb0,0xb0,0x29,0xd4,0x84,0xc9,0x4f,0x99,0x21,0xc9,0x5a,0xb3,0xed,0x06,0x9b};
    std::array<std::uint8_t,16> guid{};std::uint32_t state{};
    if(asset<0x10010 || !read(asset-16,guid) || guid!=expected ||
        !expression_owner(expression,l) || !read(l.owner.context+c::selected_state,state))return false;
    // Initial admission needs contact. Native 406 can briefly lift the board;
    // its exact actor and selector remain authoritative during that interval.
    const bool admitted=(state>=400 && state<=405 && !l.entered && contact(l)) ||
        (state==406 && active_actor(l));
    return admitted && valid(l);
}
bool start(std::uintptr_t base) noexcept {
    if(ready.load())return true;
    if(!supported_build::running_image_matches(base))return false;
    for(const auto& fp:c::functions) {
        std::array<unsigned char,32> bytes{};
        if(!read(base+fp.rva,bytes) || bytes!=fp.bytes)return false;
    }
    image=base;
    const auto target=reinterpret_cast<void*>(base+c::orientation);
    void* original{};
    if(hook_prepare(target,reinterpret_cast<void*>(&orientation),&original)!=HookOk)return false;
    original_orientation=reinterpret_cast<c::Orientation>(original);
    if(hook_queue_enable(target)!=HookOk || hook_apply_queued()!=HookOk) {
        hook_disable(target);hook_remove(target);return false;
    }
    ready.store(true);return true;
}
}
