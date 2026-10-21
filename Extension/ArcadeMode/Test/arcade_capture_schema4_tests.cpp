#include "Extension/ArcadeMode/arcade_input_telemetry.h"
#include <cstdlib>
#include <iostream>
#include <string>

using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool condition,const char* message) {
    ++checks;
    if(!condition){std::cerr<<message<<'\n';std::exit(1);}
}
CaptureBudgetReceipt receipt(std::uint64_t bytes,std::uint64_t records) {
    CaptureBudgetReceipt r;
    r.queue_valid=r.file_valid=true;r.file_id=91;r.volume=7;
    r.file_bytes=bytes;r.file_records=records;r.prefix_hash=42;
    return r;
}
CaptureCorrelationEvidence begin() {
    CaptureCorrelationEvidence r;
    r.schema=4;r.capture=11;r.required_sources=127;
    r.kind=CaptureCorrelationKind::begin;r.ms=1000;r.events_known=true;
    r.budget=receipt(1000,10);
    return r;
}
CaptureCorrelationEvidence event(unsigned kind,std::uint64_t ordinal) {
    CaptureCorrelationEvidence r;
    r.schema=4;r.capture=11;r.required_sources=127;
    r.kind=CaptureCorrelationKind::event;r.ordinal=ordinal;
    r.ms=1000+ordinal;r.events_known=true;
    r.semantic.m_kind=static_cast<std::uint8_t>(kind);
    r.source=kind==1?CaptureCorrelationSource::recognition:
        kind==2?CaptureCorrelationSource::pad_poll:
        (kind==3||kind==7)?CaptureCorrelationSource::native:
        kind==6?CaptureCorrelationSource::manual_termination:
        CaptureCorrelationSource::manual_admission;
    r.source_sequence=ordinal;
    if(kind==1){
        r.recognition_id=1;r.recognition_session=1;r.action=1;r.context=1;
        r.footprint_kind=1;r.member_count=1;r.first_event=r.last_event=1;
        r.first_ms=r.last_ms=1000;r.recognized_ms=r.ms;
    }
    if(kind==2){
        r.semantic.m_poll=ordinal;r.controller.slot=0;r.controller.selected_slot=0;
        r.controller.returned_ms=r.ms;
    }
    return r;
}
CaptureCorrelationEvidence footer(std::uint64_t expected) {
    auto r=begin();r.kind=CaptureCorrelationKind::end;r.ms=1080;
    r.expected=expected;r.ordinal=expected+1;r.complete=true;
    r.semantic.m_attempted=8;r.semantic.m_emitted=8;
    return r;
}
CaptureCorrelationEvidence run(bool omit_render=false,bool stall=false,bool burst=false,bool rotate=false) {
    CaptureCorrelationDrain drain;
    auto first=begin();check(capture_correlation_valid(first),"schema4 begin remains typed");
    drain.written(first,true);
    std::uint64_t ordinal{};
    for(unsigned kind=1;kind<=8;++kind){
        if(omit_render&&kind==8)continue;
        auto row=event(kind,++ordinal);
        check(capture_correlation_valid(row),"semantic row preserves exact source and type contract");
        drain.written(row,true);
    }
    for(auto source:{CaptureCorrelationSource::input,CaptureCorrelationSource::native_admission}){
        auto row=event(0,++ordinal);row.source=source;
        check(capture_correlation_valid(row),"ordinary rows preserve required source mask");
        drain.written(row,true);
    }
    drain.cycle(stall?1101:1050,burst?53:12);
    auto finish=receipt(1500,20);finish.attempted=ordinal;finish.emitted=ordinal;
    finish.drained=ordinal;finish.peak=burst?53:12;
    if(rotate)++finish.file_id;
    auto result=drain.footer(footer(ordinal),0,finish,stall?1102:1080);
    check(capture_correlation_valid(result),"schema4 footer remains typed after accounting");
    return result;
}
}
int main() {
    const auto good=run();
    check(good.complete&&!good.overflow&&good.semantic.m_coverage==255&&
          good.semantic.q_budget_ok&&good.semantic.log_delivery,
          "bounded queue and same active writer receipt preserve complete semantic transport");
    check(!capture_correlation_complete(good),"UNKNOWN native proof never becomes whole native acceptance");
    for(auto result:{run(true),run(false,true),run(false,false,true),run(false,false,false,true)})
        check(!result.complete&&result.overflow,"missing render, stall, burst and rotation each fail closed");
    auto wrong_poll=event(2,2);wrong_poll.semantic.m_poll=1;
    check(!capture_correlation_valid(wrong_poll),"final RS transition must carry its own poll foreign key");
    auto nonsemantic=event(0,1);nonsemantic.source=CaptureCorrelationSource::input;
    nonsemantic.semantic.m_poll=1;
    check(!capture_correlation_valid(nonsemantic),"nonsemantic row cannot acquire a poll ID");
    CaptureSchema4 native{};native.m_native_proof=1;
    check(!native.valid(),"unqualified native proof is rejected");
    native={};native.m_completion=1;
    check(!native.valid(),"unqualified native completion is rejected");
    const auto encoded=capture_schema4_suffix(CaptureSchema4{});
    check(encoded.starts_with(" m_version=1 m_kind=0 m_required=255")&&
          encoded.ends_with(" m_render_ms=0.")&&
          encoded.find("m_from_air=0")!=std::string::npos,
          "suffix uses ordered decimal fields and numeric booleans");
    InputTelemetryQueue<unsigned,64> queue;
    queue.start_accounting();
    for(unsigned i=0;i<53;++i)check(queue.push(i),"burst reaches existing bounded queue");
    check(queue.counters().peak==53,"per-reservation high-water catches an unsampled burst");
    std::array<unsigned,64> out{};
    check(queue.take(out)==53&&queue.counters().backlog==0&&queue.counters().peak==53,
          "drain cannot erase prior over-budget evidence");
    queue.stop_accounting();
    const auto counted=queue.counters().attempted;
    check(queue.push(99)&&queue.take(out)==1&&queue.counters().attempted==counted,
          "capture OFF stops the added per-arrival accounting");
    InputTelemetry telemetry;
    telemetry.begin_capture_accounting();
    PadTelemetry ordinary{};
    for(unsigned i=0;i<32;++i)check(telemetry.pad.push(ordinary),"all-traffic burst fits the physical queue");
    check(telemetry.arrival.violation()&&telemetry.counters().invalid,
          "cross-queue token overrun taints capture before the worker can sample backlog");
    telemetry.end_capture_accounting();
    InputTelemetry combined;
    combined.begin_capture_accounting();
    for(unsigned i=0;i<8;++i){
        check(combined.pad.push(ordinary),"pad arrivals count in combined budget");
        check(combined.combo.queue.push({}),"native arrivals count in combined budget");
        check(combined.client.push(ClientTelemetry{Observation{}}),"client arrivals count in combined budget");
    }
    check(combined.arrival.violation()&&combined.counters().invalid,
          "one token bucket accounts for all three existing telemetry queues");
    combined.end_capture_accounting();
    InputTelemetryQueue<unsigned,64> inherited;
    check(inherited.push(1)&&inherited.push(2),"ordinary rows can predate capture activation");
    inherited.start_accounting();
    check(inherited.take(out)==2&&inherited.counters().drained==0,
          "preexisting backlog drain cannot be charged as newly emitted capture rows");
    check(inherited.push(3)&&inherited.take(out)==1&&inherited.counters().emitted==1&&
          inherited.counters().drained==1&&!inherited.counters().overflow,
          "capture interval accounts only its own successful row");
    inherited.stop_accounting();
    std::cout<<checks<<" schema4 transport checks passed\n";
}
