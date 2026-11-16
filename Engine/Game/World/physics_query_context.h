#pragma once
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/20260929/rigid_bodies.h"
#include "Engine/Game/Build/20260929/park_editor.h"

namespace dingosdk::physics_query_context {
// A physics callback can migrate between workers. Qualify the calling worker's
// live client context, not the thread that first updated customization. The
// native query still acquires its own read locks and owns its allocation scope.
template<class Read>
bool current(std::uintptr_t base, std::uintptr_t client, std::uintptr_t tls_table,
             std::uintptr_t context, std::uintptr_t world, Read&& read) {
    namespace e=game::build::v20260929::engine;
    namespace r=game::build::v20260929::rigid_bodies;
    namespace q=game::build::v20260929::park_editor;
    constexpr std::uintptr_t highest=0x00007fffffffffffULL;
    const auto at=[&]<class T>(std::uintptr_t object,std::uintptr_t offset,T& value) {
        return object>=0x10000 && offset<=highest-sizeof(T) &&
            object<=highest-offset-sizeof(T) && read(object+offset,value);
    };
    struct Snapshot {
        std::uintptr_t table{},context{},tls{},current{};
        std::uint32_t phase{},index{};
        std::uint8_t active{};
        bool operator==(const Snapshot&) const = default;
    };
    const auto capture=[&](Snapshot& s) {
        std::uint32_t readers{},queries{};
        return base>=0x10000 && base<=highest-e::client_vtable && context>=0x10000 &&
            at(client,0,s.table) && s.table==base+e::client_vtable &&
            at(client,r::client_context,s.context) && s.context==context &&
            at(client,r::client_phase,s.phase) && (s.phase==13 || s.phase==21) &&
            at(base,e::tls_index,s.index) && s.index<=4095 &&
            at(tls_table,std::uintptr_t{s.index}*sizeof(std::uintptr_t),s.tls) &&
            at(s.tls,r::tls_active,s.active) && s.active &&
            at(s.tls,r::tls_context,s.current) && s.current==context &&
            // Do not enter from a writer-owned world and wait on ourselves.
            // These are admission observations, never a replacement for locks.
            at(world,q::query_readers,readers) && readers && readers<=q::query_reader_limit &&
            at(world,q::query_secondary_readers,queries) && queries && queries<=q::query_reader_limit;
    };
    Snapshot before{},after{};
    return capture(before) && capture(after) && before==after;
}
}
