#pragma once
#include <cstdint>

namespace dingosdk::arcade_mode {
enum class ControllerRouteReason : unsigned {
    ready, off, source_unavailable, no_connected_xinput, unsupported_source,
    multiple_connected, no_physical_sample, physical_clock_future,
    physical_sample_stale, selection_unavailable, selection_clock_future, selection_stale, binding_waiting, binding_revoked
};
constexpr const char* controller_route_reason(ControllerRouteReason reason) noexcept {
    switch(reason){
#define ARCADE_ROUTE_REASON(name) case ControllerRouteReason::name: return #name;
        ARCADE_ROUTE_REASON(ready) ARCADE_ROUTE_REASON(off) ARCADE_ROUTE_REASON(source_unavailable)
        ARCADE_ROUTE_REASON(no_connected_xinput) ARCADE_ROUTE_REASON(unsupported_source)
        ARCADE_ROUTE_REASON(multiple_connected) ARCADE_ROUTE_REASON(no_physical_sample)
        ARCADE_ROUTE_REASON(physical_clock_future) ARCADE_ROUTE_REASON(physical_sample_stale)
        ARCADE_ROUTE_REASON(selection_unavailable) ARCADE_ROUTE_REASON(selection_clock_future)
        ARCADE_ROUTE_REASON(selection_stale) ARCADE_ROUTE_REASON(binding_waiting) ARCADE_ROUTE_REASON(binding_revoked)
#undef ARCADE_ROUTE_REASON
    }
    return "invalid";
}
// Snapshot of existing routing facts only. Slot/actor4 means unavailable.
// bitmap_known=false suppresses unsupported source values (including HID
// generation tokens) and unavailable SDK reads, rather than calling them zero.
struct ControllerRouteEvidence {
    std::uint64_t ms{}, generation{};
    unsigned bitmap{}, physical_slot{4}, logical_actor{4};
    bool bitmap_known{}, enabled{}, source_available{};
    ControllerRouteReason reason{ControllerRouteReason::selection_unavailable};
    bool explicit_binding{};
};
inline bool controller_route_evidence_valid(const ControllerRouteEvidence& r) noexcept {
    if(r.bitmap>15 || (!r.bitmap_known && r.bitmap) || r.physical_slot>4 ||
        (r.logical_actor!=0 && r.logical_actor!=4) ||
        static_cast<unsigned>(r.reason)>static_cast<unsigned>(ControllerRouteReason::binding_revoked))return false;
    if(r.bitmap_known && !r.source_available)return false;
    if(r.reason==ControllerRouteReason::ready)return r.enabled&&r.source_available&&r.bitmap_known&&
        r.physical_slot<4&&(r.explicit_binding ? (r.bitmap&(1u<<r.physical_slot))!=0 : r.bitmap==(1u<<r.physical_slot))&&r.logical_actor==0;
    return r.logical_actor==4;
}
// At most one row per second, including changes; stable state refreshes every
// five seconds. This throttle never grants or withdraws controller authority.
class ControllerRouteReporting {
public:
    bool due(const ControllerRouteEvidence& row) noexcept {
        if(!controller_route_evidence_valid(row))return false;
        if(emitted_ && (row.ms<last_.ms || row.ms-last_.ms<1000))return false;
        const bool changed=!emitted_ || row.generation!=last_.generation || row.bitmap!=last_.bitmap ||
            row.physical_slot!=last_.physical_slot || row.logical_actor!=last_.logical_actor ||
            row.bitmap_known!=last_.bitmap_known || row.enabled!=last_.enabled ||
            row.source_available!=last_.source_available || row.reason!=last_.reason || row.explicit_binding!=last_.explicit_binding;
        if(!changed && row.ms-last_.ms<5000)return false;
        last_=row;emitted_=true;return true;
    }
private:
    ControllerRouteEvidence last_{};
    bool emitted_{};
};
}
