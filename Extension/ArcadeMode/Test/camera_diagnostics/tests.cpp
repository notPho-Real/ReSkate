#include "Extension/Skater/camera_diagnostics_format.h"
#include "Extension/ArcadeMode/arcade_input_telemetry.h"
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
using namespace dingosdk::camera_diagnostics;
namespace {
unsigned checks{};std::vector<Evidence> rows;bool reject_sink{};
std::atomic<bool> block_sink{},inside_sink{},release_sink{};
std::atomic<unsigned> block_kind{1};
void check(bool yes,const char* what){++checks;if(!yes){std::cerr<<what<<'\n';std::exit(1);}}
bool sink(const Evidence& row)noexcept {
    if(block_sink.load()&&row.kind==block_kind.load()){inside_sink=true;while(!release_sink.load())std::this_thread::yield();}
    if(reject_sink)return false;
    rows.push_back(row);return true;
}
void capture_bounds(){
    Capture c;Frame f;
    check(!c.arm(10,3001,0,sink),"duration bound");
    check(!c.arm(UINT64_MAX,1,0,sink),"deadline overflow");
    check(!c.arm(10,0,0,sink),"no stop without capture");
    reject_sink=true;check(!c.arm(10,100,0,sink),"failed begin unavailable");reject_sink=false;
    rows.clear();check(c.arm(100,3000,5,sink),"finite begin");check(!c.arm(101,2,5,sink),"no active rearm");
    for(unsigned i=0;i<200;++i){c.begin(f,100+i,5);c.finish(f,101+i,5);}
    c.begin(f,3101,5);c.finish(f,3101,5);
    check(rows.size()==194,"begin plus192rows plusfooter");
    check(rows.back().kind==2&&rows.back().attempted==200&&rows.back().queued==192&&rows.back().lost==8&&rows.back().truncated,"cap exhaustion explicit");
    for(const auto& row:rows){check(valid(row),"bounded row validates");check(format(row).size()<4096,"fixed line under4KiB");}
    rows.clear();check(c.arm(4000,50,9,sink),"rearm new epoch");c.begin(f,4050,9);c.finish(f,4051,10);
    check(rows.size()==3&&rows[1].late&&rows.back().client_drop_end==10,"inclusive boundary late receipt and queue drop retained");
    rows.clear();check(c.arm(5000,50,10,sink),"new capture");c.begin(f,5001,10);
    check(c.arm(5002,0,10,sink)&&rows.size()==1,"stop keeps in-flight unknown untilcompletion");
    c.finish(f,5003,10);check(rows.back().kind==2&&rows.back().attempted==1,"pending receipt thenfooter");
    rows.clear();check(c.arm(6000,50,10,sink),"new capture before failedrow");c.begin(f,6001,10);reject_sink=true;c.finish(f,6002,11);reject_sink=false;
    check(c.arm(6003,0,11,sink)&&rows.back().lost==1&&rows.back().queued==0,"failed row never counted queued");
}
void concurrent_loss(){
    rows.clear();Capture c;Frame a,b;check(c.arm(10,100,0,sink),"concurrency begin");
    c.begin(a,11,0);c.begin(b,12,0);block_sink=true;inside_sink=false;release_sink=false;
    std::thread worker([&]{c.finish(a,13,0);});
    while(!inside_sink.load())std::this_thread::yield();
    c.finish(b,14,0); // Diagnostic contention loses a receipt, never waits on gameplay.
    Frame denied;c.begin(denied,15,0);c.finish(denied,15,0);
    release_sink=true;worker.join();block_sink=false;
    check(c.arm(16,0,0,sink),"contention does not strand pending orpreventstop");
    check(rows.back().kind==2&&rows.back().lost==2&&rows.back().queued==1&&rows.back().attempted==3&&rows.back().uninspected==1,"both contention losses explicit");
}
void retirement_and_epoch(){
    rows.clear();Capture c;Frame f;
    check(c.arm(1000,100,0,sink),"short capture");
    c.writer(Writer::debug,999);c.mode(false,999);c.begin(f,999,0);c.finish(f,999,0);
    c.writer(Writer::debug,1000);c.writer(Writer::debug,1100);c.writer(Writer::debug,1101);
    c.mode(true,1000);c.mode(true,1100);c.mode(true,1101);
    c.begin(f,1001,0);c.finish(f,1002,0);
    block_kind=2;block_sink=true;inside_sink=false;release_sink=false;
    std::thread closer([&]{check(c.arm(1101,0,0,sink),"close request");});
    while(!inside_sink.load())std::this_thread::yield();
    // Footer construction follows admission closure. A late call carrying an
    // earlier timestamp cannot join that retired epoch or modify its counters.
    Frame late;c.begin(late,1005,0);c.finish(late,1006,0);
    c.writer(Writer::debug,1005);c.mode(false,1005);
    check(!c.arm(1102,100,0,sink),"no rearm while footer is pending");
    release_sink=true;closer.join();block_sink=false;block_kind=1;
    check(rows.back().attempted==1&&rows.back().queued==1&&rows.back().lost==0,"footer covers admitted attempts only");
    check(rows.back().writer_totals[0]==2&&rows.back().mode_requests[1]==2&&rows.back().mode_requests[0]==0,"exact inclusive start and end");
    rows.clear();check(c.arm(2000,50,0,sink),"rearm after retired footer");
    c.writer(Writer::debug,1099);c.mode(false,1099);c.begin(late,1099,0);c.finish(late,1099,0);
    c.writer(Writer::party,2001);c.mode(false,2001);
    c.begin(f,2002,0);c.finish(f,2003,0);check(c.arm(2004,0,0,sink),"new epoch still active after stale timestamp");
    check(rows.back().attempted==1&&rows.back().writer_totals[0]==0&&rows.back().writer_totals[2]==1&&rows.back().mode_requests[0]==1,"no cross-start or prior-epoch contamination");
}
void observation_values(){
    rows.clear();check(capture.arm(100,100,0,sink),"observation begin");capture.begin(frame,101,0);
    gate(false,true,true);reason(Reason::input);input(false,false,{},0,0,4,0,0);
    input(true,false,{},0,0,4,0,0);input(true,true,{1,2,3,4,5},7,8,1,100,-100);
    restore();const auto op=operation(false,1,true,false);selection(op,false,1,0x111100);selection(op,true,0,0x222200);stage(op,2);returned(op);restore(true);
    identity(op,{1,2,3,4,5,6,7,8,9,10,11},{1,2,3,4,5,6,7,8,9,10,11});
    phase(op,4,5,6);const std::array<float,16> m{1,0,0,0,0,1,0,0,0,0,1,0,4,5,6,1};pose(0,0x111100,2,m,60);pose(1,0,0,m,60);pose(2,0x222200,1,m,60);pose(4,0,0,m,60);root({1,2,3});
    writer(Writer::arcade,101);capture.mode(false,101);capture.finish(frame,102,0);writer(Writer::first_person,103);writer(Writer::first_person,201);capture.mode(true,201);
    check(capture.arm(103,0,0,sink),"OFF observation remains stoppable");
    const auto& r=rows[1];check(!r.enabled&&r.input_count==3,"OFF and3reachedreads retained");
    check(!r.input[0].inspected&&!r.input[0].valid&&r.input[1].inspected&&!r.input[1].valid,"uninspected distinct from invalid");
    check(r.input[2].owner>0&&r.input[2].owner_generation==7,"anonymous owner andgeneration");
    check(r.operation[0].before_camera!=r.operation[0].after_camera&&r.operation[0].returned,"verifiedselectiontransition separatefromrequest");
    check(r.operation[0].lease_identity&&r.operation[0].lease_identity==r.operation[0].observed_identity&&r.operation[0].phase_generation==5,"full anonymous lease tuple and per-operation phase");
    check(rows.back().writer_totals[3]==1,"render handoff outside tick counted separately");
    check(rows.back().mode_requests[0]==1&&rows.back().mode_requests[1]==0,"OFF observed independently and expired events excluded");
    const auto wire=format(r);check(wire.find("111100")==std::string::npos&&wire.find("222200")==std::string::npos,"raw camera addresses absent");
    check(wire.find("final_presented_verified=false")!=std::string::npos,"no final presentation claim");
    Evidence bad=r;bad.input[0].valid=true;check(!valid(bad),"uninspectedcannotvalid");bad=r;bad.view[0].eye[0]=NAN;check(!valid(bad),"nonfinite observedpose");
    bad=r;bad.sequence=193;check(!valid(bad),"sequencecap");bad=r;bad.end_ms=100;check(!valid(bad),"clockregression");
    bad=r;bad.input[2].owner=0;check(!valid(bad),"valid input requires anonymous owner");
    bad=r;bad.operation[0].stage=5;check(!valid(bad),"unknown adapter stage rejected");
    rows.clear();check(capture.arm(200,100,0,sink),"nested capture");capture.begin(frame,201,0);capture.begin(frame,202,0);input(true,true,{9,8,7,6,5},8,9,1,1,1);capture.finish(frame,203,0);capture.finish(frame,204,0);capture.arm(205,0,0,sink);
    check(rows[1].nested&&rows[1].input_count==0,"nested observations do not borrow outer frame");
}
void actual_queue(){
    using namespace dingosdk::arcade_mode;
    InputTelemetryQueue<ClientTelemetry,64> q;Evidence r;r.capture=1;r.kind=1;r.sequence=1;r.ms=r.end_ms=1;r.duration=100;
    for(unsigned i=0;i<64;++i)check(q.push(ClientTelemetry{r}),"actual camera queue fill");
    check(!q.push(ClientTelemetry{r})&&q.dropped()==1,"actualfullqueuekeepsloss");
    std::array<ClientTelemetry,16> out;check(q.take(out)==16,"unchanged16rowfairbatch");
    check(sizeof(Evidence)<=1008&&sizeof(ClientTelemetry)==1016,"variantfootprintunchanged");
    std::cout<<"Evidence="<<sizeof(Evidence)<<" variant="<<sizeof(ClientTelemetry)<<" queue="<<sizeof(q)<<'\n';
}
}
int main(){capture_bounds();concurrent_loss();retirement_and_epoch();observation_values();actual_queue();std::cout<<checks<<" camera observation checks passed\n";}
