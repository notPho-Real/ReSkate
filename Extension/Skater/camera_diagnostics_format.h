#pragma once
#include "camera_diagnostics.h"
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

namespace dingosdk::camera_diagnostics {
// Worker only. Compact fixed numeric grammar; no addresses, paths or native text.
inline std::string format(const Evidence& r) {
    std::ostringstream s;s.imbue(std::locale::classic());s<<std::setprecision(9);
    s<<"ARCADE CAMERA: schema=1 capture="<<r.capture<<" seq="<<r.sequence<<" kind="<<r.kind
     <<" ms="<<r.ms<<" end_ms="<<r.end_ms<<" duration="<<r.duration
     <<" phase_sample="<<r.phase_sample<<" phase_gen="<<r.phase_generation<<" phase_calls="<<r.phase_calls
     <<" phase_known="<<r.phase_known<<" enabled="<<r.enabled<<" ready="<<r.ready<<" phase="<<r.phase
     <<" reason="<<static_cast<unsigned>(r.reason)<<" inputs="<<r.input_count<<" operations="<<r.operation_count
     <<" restores="<<r.restore_requests<<" restored="<<r.restore_successes<<" writers="<<r.writer_mask;
    for(unsigned n=0;n<4;++n)s<<" writer"<<n<<'='<<r.writer_totals[n];
    s<<" off_requests="<<r.mode_requests[0]<<" on_requests="<<r.mode_requests[1];
    for(unsigned n=0;n<3;++n){const auto& i=r.input[n];
        s<<" input"<<n<<'='<<i.inspected<<','<<i.valid<<','<<i.owner<<','<<i.owner_generation<<','<<i.route_generation<<','<<i.slot<<','<<i.x<<','<<i.y;
        const auto& o=r.operation[n];
        s<<" op"<<n<<'='<<o.requested<<','<<o.transform<<','<<o.requester<<','<<o.owned<<','<<o.uncertain<<','<<o.stage<<','<<o.before_known<<','<<o.before_mode<<','<<o.before_camera<<','<<o.after_known<<','<<o.after_mode<<','<<o.after_camera<<','<<o.returned
         <<','<<o.lease_identity<<','<<o.observed_identity<<','<<o.phase_sample<<','<<o.phase_generation<<','<<o.phase_calls;
        const auto& p=r.view[n];
        s<<" pose"<<n<<'='<<p.valid<<','<<p.kind<<','<<p.camera<<','<<p.eye[0]<<','<<p.eye[1]<<','<<p.eye[2]<<','<<p.back[0]<<','<<p.back[1]<<','<<p.back[2]<<','<<p.fov;
    }
    const auto& f=r.follow;
    s<<" follow="<<f.valid<<','<<f.eye[0]<<','<<f.eye[1]<<','<<f.eye[2]<<','<<f.back[0]<<','<<f.back[1]<<','<<f.back[2]<<','<<f.fov;
    s<<" root_known="<<r.root_known<<" root="<<r.root[0]<<','<<r.root[1]<<','<<r.root[2]
     <<" attempted="<<r.attempted<<" queued="<<r.queued<<" lost="<<r.lost
     <<" uninspected="<<r.uninspected
     <<" pending="<<r.pending
     <<" client_drop_begin="<<r.client_drop_begin<<" client_drop_end="<<r.client_drop_end
     <<" nested="<<r.nested<<" truncated="<<r.truncated<<" late="<<r.late<<" final_presented_verified=false.";
    return s.str();
}
}
