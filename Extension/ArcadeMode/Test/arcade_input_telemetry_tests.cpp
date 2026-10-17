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
int main() {
    saturation_and_recovery();
    transition_and_startup();
    client_rows();
    score_probe_rows();
    concurrent_producers();
    std::cout << checks << " input telemetry checks passed\n";
}
