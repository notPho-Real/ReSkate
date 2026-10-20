#include "Extension/ArcadeMode/arcade_input_telemetry.h"
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

namespace {
using namespace dingosdk::arcade_mode;
unsigned checks{};
void check(bool value, const char* message) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Row { std::uint64_t producer{}, ordinal{}, checksum{}; };
constexpr std::uint64_t checksum(std::uint64_t producer, std::uint64_t ordinal) {
    return (producer * 1000003u) ^ ordinal ^ 0xabcdef12u;
}
void saturation_and_recovery() {
    BoardMotionEvidence motion{1, 500, 1.0f / 60, 3, 0, 4, 100, 1, 0x40, 0xc000, true, false};
    check(client_telemetry_valid(motion), "native60Hz velocity and step time fit the bounded telemetry channel");
    motion.seconds = 1.0f / 30;
    check(client_telemetry_valid(motion), "separate native30Hz observations retain their actual timestep");
    motion.seconds = 0;
    check(!client_telemetry_valid(motion), "paused or missing physics time cannot become a motion sample");
    motion.seconds = .017f; motion.vx = std::numeric_limits<float>::infinity();
    check(!client_telemetry_valid(motion), "invalid native velocity is rejected before formatting");
    InputTelemetryQueue<Row, 4> queue;
    std::array<Row, 4> out{};
    check(queue.take(out) == 0 && queue.take_dropped() == 0, "empty worker drain is harmless");
    for (unsigned i = 0; i < 4; ++i) check(queue.push({1, i, checksum(1, i)}), "capacity accepts ordered rows");
    for (unsigned i = 0; i < 1000; ++i) check(!queue.push({2, i, 0}), "paused consumer bounds memory and rejects immediately");
    check(queue.dropped() == 1000 && queue.take_dropped() == 1000, "every rejected row is counted");
    check(queue.take_dropped() == 0 && queue.dropped() == 1000, "gap deltas do not reset cumulative losses");
    check(queue.take(std::span<Row>(out).first(2)) == 2 && out[0].ordinal == 0 && out[1].ordinal == 1,
        "bounded drain preserves accepted order");
    check(queue.push({3, 4, checksum(3, 4)}) && queue.push({3, 5, checksum(3, 5)}), "freed slots accept without resetting queue");
    check(!queue.push({4, 6, 0}) && queue.take_dropped() == 1 && queue.dropped() == 1001, "loss accounting survives wrap and recovery");
    check(queue.take(out) == 4 && out[0].ordinal == 2 && out[1].ordinal == 3 && out[2].ordinal == 4 && out[3].ordinal == 5,
        "wrapping never overwrites unread evidence");
    for (unsigned lap = 0; lap < 100; ++lap) {
        Row source{4, lap, checksum(4, lap)};
        check(queue.push(source), "queue remains usable across repeated fill/drain lifecycle");
        source = {};
        check(queue.take(out) == 1 && out[0].checksum == checksum(4, lap), "publication owns its snapshot, not caller storage");
    }
}
void transition_and_startup() {
    InputTelemetryLink link;
    PadTelemetry pad{}; pad.pad.sequence = 31; pad.pad.ms = 300; pad.pad.source = true;
    check(!link.pad(pad), "failed or absent worker startup cannot accept an unconsumed row");
    ComboSample sample{{true, 100, 1, {}}, 200, {true, 2}};
    link.combo(sample);
    InputTelemetry state;
    link.publish(state);
    std::array<ComboSample, 8> combos{};
    check(state.combo.queue.take(combos) == 0, "no pre-start observations are replayed");
    link.combo(sample);
    sample.ms += 1; link.combo(sample);
    check(state.combo.queue.take(combos) == 1 && combos[0].ms == 200, "duplicate physics samples remain suppressed");
    sample.sample.state = 600; sample.ms = 202; link.combo(sample);
    sample.sample.state = 100; sample.ms = 203; link.combo(sample);
    sample.flip.id = 3; sample.ms = 204; link.combo(sample);
    check(state.combo.queue.take(combos) == 3 && combos[0].sample.state == 600 &&
        combos[1].sample.state == 100 && combos[2].flip.id == 3, "short native transitions and independent flip changes survive");
    check(link.pad(pad), "successful worker publication enables fixed snapshots");
    PadTelemetry diagnostic{}; diagnostic.diagnostic = true; diagnostic.request = {false, true, false, 2, 7};
    check(link.pad(diagnostic), "remap diagnostics use the same bounded channel");
    pad.pad.sequence = 32; pad.pad.source = false; pad.pad.ms = 301; pad.pad.action_sample = true;
    pad.pad.left_x = -12345; pad.pad.right_y = 32767; pad.pad.output_packet = 123; pad.pad.output_rt = 255;
    check(link.pad(pad), "following sample is retained");
    std::array<PadTelemetry, 4> pads{};
    check(state.pad.take(pads) == 3 && pads[0].pad.sequence == 31 && pads[0].pad.source &&
        pads[1].diagnostic && pads[1].request.requested_id == 7 && pads[2].pad.sequence == 32 &&
        pads[2].pad.ms == 301 && pads[2].pad.left_x == -12345 && pads[2].pad.right_y == 32767 &&
        pads[2].pad.output_packet == 123 && pads[2].pad.output_rt == 255,
        "source, diagnostic, and delivered pad rows retain FIFO order and signed fields");
    ComboTelemetry<2> tiny;
    tiny.observe(sample); ++sample.sample.state; tiny.observe(sample); ++sample.sample.state; tiny.observe(sample);
    check(tiny.queue.dropped() == 1 && tiny.queue.take(combos) == 2, "physics transition saturation is explicit");
    ++sample.sample.state; tiny.observe(sample);
    check(tiny.queue.take(combos) == 1 && combos[0].sample.state == sample.sample.state,
        "physics evidence recovers after saturation without replaying a failed transition");
}
void client_rows() {
    FootMotionEvidence foot{1, 500, 450, 1, true, true, false, false, 47.5f, 3.2f};
    check(client_telemetry_valid(foot), "independent native foot flags and fresh retained position can be published");
    foot.position_ms = 349;
    check(!client_telemetry_valid(foot), "stale native position cannot prove jump displacement");
    foot.position_ms = 501;
    check(!client_telemetry_valid(foot), "future position fails the shared motion provenance contract");
    foot.position_ms = 450; foot.vy = std::numeric_limits<float>::quiet_NaN();
    check(!client_telemetry_valid(foot), "nonfinite native pelvis motion fails closed");
    foot.vy = 0; foot.valid = false; foot.position_ms = 0;
    check(client_telemetry_valid(foot), "explicit invalidation survives without invented position provenance");
    InputTelemetry state;
    InputTelemetryLink link;
    HeartbeatEvidence heartbeat{};
    heartbeat.sequence = 11; heartbeat.ms = 500; heartbeat.valid = true;
    heartbeat.state = 100; heartbeat.mode = 1; heartbeat.speed = 3.25f;
    heartbeat.score = 12345; heartbeat.multiplier = 7; heartbeat.score_valid = true;
    check(!link.client(heartbeat), "client evidence also remains disabled after absent worker startup");
    check(client_telemetry_fresh(heartbeat, 500) && client_telemetry_fresh(heartbeat, 600), "heartbeat queue age includes exact100ms boundary");
    check(!client_telemetry_fresh(heartbeat, 499) && !client_telemetry_fresh(heartbeat, 601), "clock reversal and stale queued heartbeats cannot appear freshly sampled");
    link.publish(state);
    const std::array<ClientTelemetry, 11> rows{Observation{true, 400, 51, 2},
        RevertLinkEvidence{500, 8, 4, true, 750}, BalanceFailureEvidence{501, 2, 17},
        BalanceSampleEvidence{502, 1, -.8f, -.2f, .4f, .6f, -32768, 32767},
        ComboEventEvidence{503, 3, true, false, .1f, .2f, .3f}, BalanceBailEvidence{504, 17, 3},
        ScoreEdgeEvidence{9, 505, 504, 4, 1}, heartbeat, NativeCaptureEvidence{16, true},
        NativeSampleEvidence{16, 25, {true, 200, 1, {1, 2, 3, 4, 5, 6, 7, 8}}},
        InputEdgeEvidence{'X', 10, {true, 200, 9, 1}}};
    for (const auto& row : rows) check(link.client(row), "all numeric event shapes fit the bounded channel");
    std::array<ClientTelemetry, 16> out{};
    check(state.client.take(out) == rows.size(), "one ordered queue carries all client event shapes");
    bool ordered = true;
    for (std::size_t i = 0; i < rows.size(); ++i) ordered = ordered && rows[i].index() == out[i].index();
    check(ordered, "native capture and state transitions retain client ordering");
    const auto& meter = std::get<BalanceSampleEvidence>(out[3]);
    check(meter.x == -.8f && meter.v == -.2f && meter.raw_x == -32768 && meter.raw_y == 32767,
        "queue preserves signed model position/velocity and input without formatting or quantization");
    const auto& beat = std::get<HeartbeatEvidence>(out[7]);
    check(beat.sequence == 11 && beat.ms == 500 && beat.score == 12345 && beat.multiplier == 7 &&
        beat.speed == 3.25f && beat.state == 100, "paired native state/speed capture and score fields remain coherent");
    check(std::get<NativeSampleEvidence>(out[9]).sample.words[7] == 8 &&
        std::get<InputEdgeEvidence>(out[10]).button == 'X', "bounded native words and button identity are copied exactly");
    check(client_telemetry_fresh(out[3], 100000), "historical model evidence retains its original timestamp rather than silently expiring");
    for (unsigned i = 0; i < 64; ++i) check(link.client(heartbeat), "client capacity is fixed");
    check(!link.client(heartbeat) && state.client.take_dropped() == 1, "full client channel reports a parsable gap");
    check(state.pad.dropped() == 0 && state.combo.queue.dropped() == 0, "client saturation cannot consume pad or native sample capacity");
}
void score_probe_rows() {
    ScoreProbePath root{{0x9b74fc7c}, 1};
    check(score_probe_path_text(root).view() == "9b74fc7c", "schema root path matches the existing wire representation");
    auto path = root;
    check(path.append(0) && path.append(0xffffffff) && path.append(0x123ab), "four numeric path segments are accepted");
    const auto text = score_probe_path_text(path);
    check(text.view() == "9b74fc7c.00000000.ffffffff.000123ab" && text.size == 35,
        "maximum path retains lowercase fixed-width hex, zero indexes and separators");
    const auto before = path;
    check(!path.append(9) && path == before, "fifth segment cannot overflow or mutate a completed path");
    auto sibling = root; sibling.append(1);
    auto zero = root; zero.append(0);
    check(sibling != zero && root != zero && (sibling < zero || zero < sibling), "fixed discovery keys preserve parent and sibling identity");
    ScoreProbePath invalid{};
    check(!invalid.append(3) && score_probe_path_text(invalid).view().empty(), "empty paths cannot acquire an invented root");
    invalid.count = 5;
    check(!invalid.append(3) && score_probe_path_text(invalid).view().empty(), "malformed paths never index outside the bounded array");
    InputTelemetry state;
    InputTelemetryLink link; link.publish(state);
    check(!link.client(ScoreNodeEvidence{invalid, 2, 8}) && !link.client(ScoreValueEvidence{1, invalid, 4}),
        "malformed path rows fail closed before publication");
    check(state.client.take_dropped() == 2, "rejected schema evidence remains visible as loss");
    check(link.client(ScorePresenceEvidence{100, 1, 5, 0, 1, 4}) &&
        link.client(ScoreNodeEvidence{path, 7, 8}) &&
        link.client(ScoreValueEvidence{101, path, 27, true, true}), "presence/node/value rows use the existing client channel");
    std::array<ClientTelemetry, 3> out{};
    check(state.client.take(out) == 3, "bounded scoring evidence retains its capture order");
    const auto& presence = std::get<ScorePresenceEvidence>(out[0]);
    const auto& node = std::get<ScoreNodeEvidence>(out[1]);
    const auto& value = std::get<ScoreValueEvidence>(out[2]);
    check(presence.ms == 100 && presence.rows == 5 && presence.labels == 4 &&
        node.path == path && node.kind == 7 && node.size == 8 && value.ms == 101 &&
        value.path == path && value.value == 27 && value.label && value.limit,
        "sanitized counts, path, label and discovery-limit markers survive detached publication");
}
void capture_correlation_rows() {
    CaptureCorrelationEvidence row{};
    row.capture=1; row.ms=100; row.native_ms=100; row.owner=1;
    row.owner_generation=1; row.contact_generation=1; row.action_generation=1; row.physics_step=1;
    row.native_valid=true; row.events_known=true;
    check(capture_correlation_valid(row), "capture boundary preserves original immutable proof");
    row.kind=CaptureCorrelationKind::event; row.source=CaptureCorrelationSource::input;
    row.ordinal=1; row.source_sequence=1; row.first_event=5; row.last_event=6;
    row.first_ms=90; row.last_ms=99; row.recognized_ms=100;
    check(capture_correlation_valid(row), "event footprint retains original first last and recognition times");
    const auto original=row;
    row.ms=250;
    check(capture_correlation_valid(row), "original native lease includes exact150ms boundary");
    row.ms=251;
    check(!capture_correlation_valid(row), "native proof cannot be renewed by a delayed producer");
    row=original; row.native_ms=101;
    check(!capture_correlation_valid(row), "future proof rejected");
    row=original; row.source=CaptureCorrelationSource::native; row.ms=101;
    check(!capture_correlation_valid(row), "native observation time cannot be restamped");
    row=original; row.recognized_ms=98;
    check(!capture_correlation_valid(row), "recognition cannot precede its final physical edge");
    row=original; row.events_known=false;
    check(!capture_correlation_valid(row), "unavailable footprints cannot retain invented edge fields");
    row=original; row.owner=UINT64_MAX;
    check(!capture_correlation_valid(row), "anonymous owner token remains bounded");
    row=original; row.source=static_cast<CaptureCorrelationSource>(6);
    check(!capture_correlation_valid(row), "unknown source enums rejected");
    row=original; row.kind=CaptureCorrelationKind::end; row.source=CaptureCorrelationSource::boundary;
    row.source_sequence=0; row.ordinal=2; row.first_event=0; row.last_event=0;
    row.first_ms=0; row.last_ms=0; row.recognized_ms=0;
    row.expected=1; row.drained=1; row.first_ordinal=1; row.last_ordinal=1; row.complete=true;
    check(capture_correlation_complete(row), "footer consistency requires explicit exact accounting");
    row.drop_end=1;
    check(!capture_correlation_complete(row), "shared channel losses conservatively prevent completeness");
    row.drop_end=0; row.native_valid=false;
    check(capture_correlation_valid(row) && !capture_correlation_complete(row), "unavailable native proof stays diagnostic only");
    row.native_valid=true; row.overflow=true;
    check(!capture_correlation_complete(row), "counter overflow prevents completeness");
    std::uint64_t count=UINT64_MAX-1, delta=99;
    check(capture_correlation_add(count) && count==UINT64_MAX, "checked counter accepts last representable value");
    check(!capture_correlation_add(count) && count==UINT64_MAX, "counter saturates instead of rolling over");
    check(!capture_correlation_delta(UINT64_MAX, 0, delta) && delta==0, "drop counter reset or wrap is loss");
    check(capture_correlation_delta(9, 12, delta) && delta==3, "bounded drop delta preserves exact losses");
    InputTelemetry state; InputTelemetryLink link; link.publish(state);
    row=original;
    for (unsigned i=0; i<64; ++i) check(link.client(row), "capture reuses existing bounded client capacity");
    check(!link.client(row) && state.client.take_dropped()==1, "capture loss is counted on existing channel");
    row.owner=2;
    std::array<ClientTelemetry,64> out{};
    check(state.client.take(out)==64 && std::get<CaptureCorrelationEvidence>(out[0]).owner==1,
        "queued capture rows own detached copies of original owner proof");
    row=original; row.native_ms=101;
    check(!link.client(row) && state.client.take_dropped()==1, "invalid capture attempt contributes to drop accounting");
}
void concurrent_producers() {
    constexpr unsigned producers = 4, iterations = 20000;
    InputTelemetryQueue<Row, 64> queue;
    std::atomic<bool> start{};
    std::atomic<unsigned> finished{};
    std::atomic<std::uint64_t> accepted{};
    std::vector<std::thread> threads;
    for (unsigned producer = 0; producer < producers; ++producer) threads.emplace_back([&, producer] {
        while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
        std::uint64_t count{};
        for (unsigned i = 1; i <= iterations; ++i)
            if (queue.push({producer, i, checksum(producer, i)})) ++count;
        accepted.fetch_add(count, std::memory_order_relaxed);
        finished.fetch_add(1, std::memory_order_release);
    });
    start.store(true, std::memory_order_release);
    std::array<std::uint64_t, producers> previous{};
    std::array<Row, 7> out{};
    std::uint64_t received{}, reported{};
    bool ordered = true, intact = true;
    for (;;) {
        const auto n = queue.take(out);
        reported += queue.take_dropped();
        for (std::size_t i = 0; i < n; ++i) {
            const auto& row = out[i];
            if (row.producer >= producers || row.checksum != checksum(row.producer, row.ordinal)) { intact = false; continue; }
            ordered = ordered && row.ordinal > previous[row.producer];
            previous[row.producer] = row.ordinal;
            ++received;
        }
        if (finished.load(std::memory_order_acquire) == producers && !n) break;
        std::this_thread::yield();
    }
    for (auto& thread : threads) thread.join();
    reported += queue.take_dropped();
    check(intact && ordered, "concurrent producers never expose partial rows or reorder a producer stream");
    check(received == accepted.load() && received + queue.dropped() == producers * iterations,
        "every concurrent attempt is delivered exactly once or explicitly dropped");
    check(reported == queue.dropped(), "concurrent drop deltas neither lose nor duplicate counts");
    check(queue.push({0, iterations + 1, checksum(0, iterations + 1)}) && queue.take(out) == 1,
        "queue remains live after concurrent contention");
}
}
namespace {
NativeInputFrame capture_frame(std::uint64_t at=1000,std::uint64_t step=10){
    NativeInputFrame f;f.valid=true;f.owner={11,12,13,14,15};f.owner_generation=f.contact_generation=f.action_generation=1;f.physics_step=step;f.sampled_at=at;return f;
}
void recognition_v2_rows(){
    CaptureCorrelationEvidence row;row.schema=2;row.capture=1;row.kind=CaptureCorrelationKind::event;row.source=CaptureCorrelationSource::recognition;
    row.required_sources=31;row.ordinal=row.source_sequence=1;row.ms=row.recognized_ms=1034;row.native_ms=1017;row.owner=1;
    row.owner_generation=row.contact_generation=row.action_generation=1;row.physics_step=2;row.native_valid=row.events_known=true;
    row.recognition_id=7;row.recognition_session=3;row.action=3;row.context=2;row.footprint_kind=1;row.member_count=2;
    row.first_event=2;row.last_event=8;row.first_ms=1000;row.last_ms=1034;row.held_reuse=2;
    check(capture_correlation_valid(row),"v2 summary retains exact membership count and original held footprint bounds");
    row.member_index=1;row.member_event=8;check(capture_correlation_valid(row),"matcher order may put newest edge before reused older edge");
    row.member_index=2;row.member_event=2;check(capture_correlation_valid(row),"held reuse keeps its real old event identity");
    const auto valid=row;row.member_event=9;check(!capture_correlation_valid(row),"member cannot lie outside original exact bounds");
    row=valid;row.member_index=3;check(!capture_correlation_valid(row),"member index cannot exceed count");
    row=valid;row.recognition_id=0;check(!capture_correlation_valid(row),"exhausted recognition identity cannot become a valid transport row");
    row=valid;row.source=CaptureCorrelationSource::input;check(!capture_correlation_valid(row),"other sources cannot carry invented recognition metadata");
    row=valid;row.schema=1;check(!capture_correlation_valid(row),"legacy bounds must not pretend to carry v2 membership");
    row=valid;row.footprint_kind=2;row.member_index=row.member_count=row.member_event=row.held_reuse=row.first_event=row.last_event=row.first_ms=row.last_ms=0;
    check(capture_correlation_valid(row),"known input-free default has recognition time but no fabricated physical events");
    row.first_event=row.last_event=1;check(!capture_correlation_valid(row),"input-free evidence cannot carry physical event bounds");row.first_event=row.last_event=0;
    row.footprint_kind=0;row.events_known=false;check(capture_correlation_valid(row),"unknown footprint remains explicit unavailable data");
    row.events_known=true;check(!capture_correlation_valid(row),"unknown footprint cannot be relabeled known empty");
    for(bool coherent:{false,true}){
        CaptureCorrelationProducer producer;std::vector<CaptureCorrelationEvidence> rows;auto sink=[&](const auto& r){rows.push_back(r);return true;};
        const auto frame=capture_frame();producer.begin(1000,100,2,frame,0,sink);
        auto authority=capture_frame(1034,12);authority.action_generation=2;
        auto original=InputNativeContext::from_frame(authority);if(!coherent)original.generations.action=1;
        row=valid;row.ms=row.recognized_ms=1034;row.source_sequence=1;row.ordinal=0;
        producer.recognition(row,original,true,1034,authority,0,sink);
        check(rows.back().native_valid==coherent,"same-step proof cannot mix parent lineage generation with a newer observation clock");
        check(rows.back().native_ms==1034&&rows.back().physics_step==12&&rows.back().action_generation==(coherent?2u:1u),"proof fields are original values even when explicitly unavailable");
    }
}

void capture_producer_drain(){
    {
        CaptureCorrelationProducer producer;CaptureCorrelationDrain drain;std::vector<CaptureCorrelationEvidence> rows;
        auto sink=[&](const auto& r){rows.push_back(r);return true;};const auto f=capture_frame();
        producer.begin(1000,100,4,f,0,sink);CaptureCorrelationEvidence event;event.source=CaptureCorrelationSource::native;event.source_sequence=10;event.ms=1000;event.events_known=true;
        producer.event(event,f,0,sink);producer.omitted(3);producer.end(1010,f,0,sink);
        drain.written(rows[0],true);drain.written(rows[1],true);const auto end=drain.footer(rows[2],0);
        check(end.expected==4&&end.drained==1&&end.dropped==3&&!end.complete&&!end.events_known,
            "missing diagnostic attempts count exactly once without fabricated event rows");
    }

    for(unsigned fault=0;fault<8;++fault){
        InputTelemetry telemetry;InputTelemetryLink link;link.publish(telemetry);CaptureCorrelationProducer producer;CaptureCorrelationDrain drain;
        unsigned attempts{};std::vector<CaptureCorrelationEvidence> written;
        auto sink=[&](const CaptureCorrelationEvidence& r){
            check(capture_correlation_valid(r),"actual producer constructs structurally valid immutable rows");
            ++attempts;
            if((fault==1&&r.kind==CaptureCorrelationKind::begin)||(fault==2&&r.kind==CaptureCorrelationKind::event&&r.source==CaptureCorrelationSource::input)){
                telemetry.client.lose();return false;
            }
            return link.client(r);
        };
        const auto f=capture_frame();check(producer.begin(1000,100,5,f,link.client_dropped(),sink),"explicit finite capture begins with actual full-owner native proof");
        check(!producer.begin(1001,100,5,f,0,sink),"active capture cannot be restarted or reuse its ID");
        CaptureCorrelationEvidence native;native.ms=1000;native.source=CaptureCorrelationSource::native;native.source_sequence=f.physics_step;native.events_known=true;
        producer.event(native,f,link.client_dropped(),sink);
        CaptureCorrelationEvidence input;input.ms=1000;input.source=CaptureCorrelationSource::input;input.source_sequence=51;input.first_event=input.last_event=51;input.first_ms=input.last_ms=1000;input.events_known=fault!=3;
        if(!input.events_known)input.first_event=input.last_event=input.first_ms=input.last_ms=0;
        producer.event(input,f,link.client_dropped(),sink);
        if(fault==4)telemetry.client.lose();
        auto end=f;
        if(fault==5){++end.owner_generation;end.owner.core=99;}
        if(fault==6)producer.lost();
        producer.end(1010,end,link.client_dropped(),sink);
        check(!producer.active()&&attempts==4,"begin/two attempts/end are finite and end retains attempted event count");
        std::array<ClientTelemetry,64> rows;const auto n=telemetry.client.take(rows);
        for(std::size_t i=0;i<n;++i){
            auto row=std::get<CaptureCorrelationEvidence>(rows[i]);
            if(row.kind==CaptureCorrelationKind::end)row=drain.footer(row,telemetry.client.dropped());
            const bool accepted=!(fault==7&&row.kind==CaptureCorrelationKind::event&&row.source==CaptureCorrelationSource::input);
            if(row.kind!=CaptureCorrelationKind::end)drain.written(row,accepted);
            if(accepted)written.push_back(row);
        }
        const auto& footer=written.back();check(footer.kind==CaptureCorrelationKind::end&&footer.ms==1010&&footer.native_ms==1000&&footer.physics_step==10,"drain never restamps original end/native time or physics step");
        check(capture_correlation_complete(footer)==(fault==0),"only complete boundaries, written events, known footprints, owner continuity and no losses can pass");
        check(footer.expected==2&&footer.drained==(fault==2||fault==7?1u:2u)&&footer.dropped==footer.expected-footer.drained,"footer counts actual accepted sink writes including validation/enqueue and sink failures");
        if(fault==5)check(!footer.native_valid&&footer.owner_generation==1,"owner change invalidates capture without substituting a late owner proof");
        check(producer.begin(1020,100,5,capture_frame(1020,11),link.client_dropped(),sink),"new explicit capture can start after retirement with a distinct ID");
    }
    {
        CaptureCorrelationProducer producer;std::vector<CaptureCorrelationEvidence> rows;auto sink=[&](const auto& row){rows.push_back(row);return true;};auto f=capture_frame();
        check(!producer.begin(1000,0,31,f,0,sink)&&!producer.begin(1000,5001,31,f,0,sink)&&!producer.begin(1000,10,128,f,0,sink),"duration/scope are explicitly bounded");
        check(producer.begin(1000,100,4,f,0,sink),"native-only scope uses the existing channel");
        CaptureCorrelationEvidence row;row.source=CaptureCorrelationSource::native;row.source_sequence=10;row.ms=1000;row.events_known=true;producer.event(row,f,0,sink);
        producer.maintain(1101,capture_frame(1101,11),0,sink);check(!producer.active()&&rows.back().ms==1101&&rows.back().native_ms==1101,"finite expiry uses actual callback time and its actual proof");
        const auto prior=rows.front().capture;check(producer.begin(1110,100,4,capture_frame(1110,12),0,sink)&&rows.back().capture>prior,"capture IDs are unique and monotonically increasing");
        producer.maintain(1300,{},0,sink);check(!producer.active()&&!rows.back().native_valid,"missing/stale native proof cannot renew capture or create synthetic completeness");
    }
}
}
void run_controller_capture_tests();
int main() {
    saturation_and_recovery();
    transition_and_startup();
    client_rows();
    score_probe_rows();
    capture_correlation_rows();
    capture_producer_drain();
    recognition_v2_rows();
    concurrent_producers();
    run_controller_capture_tests();
    std::cout << checks << " input telemetry checks passed\n";
}
