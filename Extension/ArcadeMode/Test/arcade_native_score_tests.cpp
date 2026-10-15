#include "Extension/ArcadeMode/arcade_native_score_buffer.h"
#include "Extension/ArcadeMode/arcade_score_boundary.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <thread>
#include <fstream>
#include <sstream>

namespace {
using namespace dingosdk::arcade_mode::native_score_detail;
unsigned checks{};
void check(bool ok, const char* description) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
struct Fixture {
    std::array<unsigned char, 8192> bytes{};
    std::size_t next{};
    std::array<std::uintptr_t, 4> objects{}, records{}, tables{};
    std::uintptr_t number{}, boolean{}, payload{};
    template<class T> void put(std::uintptr_t at, T value) {
        std::memcpy(bytes.data() + at - 0x10000, &value, sizeof(value));
    }
    template<class T> bool operator()(std::uintptr_t at, T& value) const noexcept {
        if (at < 0x10000 || at - 0x10000 > bytes.size() - sizeof(value)) return false;
        std::memcpy(&value, bytes.data() + at - 0x10000, sizeof(value)); return true;
    }
    std::uintptr_t alloc(std::size_t size) { auto at = next + 0x10000; next += (size + 7) & ~7ULL; return at; }
    std::uintptr_t type(std::uint32_t hash, unsigned kind, std::uint16_t size, std::uintptr_t& record) {
        auto object = alloc(8); record = alloc(0x68); put(object, record);
        put(record, hash); put(record + 4, static_cast<std::uint16_t>(kind << 5)); put(record + 6, size);
        return object;
    }
    void add(unsigned parent, unsigned index, std::uint32_t hash, std::uint16_t offset, std::uintptr_t type) {
        const auto at = tables[parent] + index * 24;
        put(at, hash); put(at + 8, offset); put(at + 16, type);
    }
    Fixture() {
        for (unsigned i = 0; i < 4; ++i) {
            objects[i] = type(type_hashes[i], 2, i % 2 ? 16 : 64, records[i]);
            tables[i] = alloc(48); put(records[i] + 0x60, tables[i]);
            put(records[i] + 0x2a, static_cast<std::uint16_t>(i % 2 ? 1 : 2));
        }
        std::uintptr_t ignored{};
        number = type(123, 15, 4, ignored); boolean = type(124, 10, 1, ignored);
        add(0, 0, 0xcdfa75f5, 8, objects[1]); add(0, 1, 0x9f0bcbf5, 40, number);
        add(1, 0, 0x40bff667, 4, number);
        add(2, 0, 3982548569U, 8, objects[3]); add(2, 1, 3611278405U, 40, boolean);
        add(3, 0, 2099127164U, 4, number);
        payload = alloc(64);
    }
};
}
int main(int argc, char** argv) {
    if (argc == 3 && std::string_view(argv[1]) == "--replay-unowned") {
        std::ifstream input(argv[2]);
        check(input.good(), "recorded capture file available");
        std::string line; unsigned count{};
        while (std::getline(input, line)) {
            check(++count <= 4096, "recorded replay bounded");
            std::istringstream row(line);
            unsigned trial{}, route{}, success{}, owner{}; Event event{};
            check(static_cast<bool>(row >> trial >> event.serial >> event.sampled_at >> event.epoch >> route >>
                event.payload.score >> event.payload.reason >> success >> owner), "exact numeric capture columns");
            row >> std::ws;
            check(row.eof() && trial < 64 && route >= 1 && route <= 4 && success <= 1 && owner == 0 &&
                  event.serial && event.epoch && score_valid(event.payload.score), "unmodified unowned capture schema");
            event.route = static_cast<Route>(route); event.payload.success = success != 0;
            event.owner_verified = false;
            // The recordings provide no current ownership, native occurrence or
            // raw-point proof. Missing context stays missing; capture time is
            // not repurposed as a freshly measured delivery timestamp.
            dingosdk::arcade_mode::ScoreBoundary gate;
            const auto receipt = gate.observe(event, {}, {});
            check(receipt.decision != dingosdk::arcade_mode::ScoreDecision::valid && !receipt.points,
                  "recorded native score cannot cross the monetary boundary without proof");
        }
        check(input.eof() && count > 0, "recorded replay complete and nonempty");
        std::cout << count << " recorded native captures rejected as monetary authority; ownership remains UNVERIFIED\n";
        return 0;
    }
    check(argc == 1, "only tests or recorded replay supported");
    for (const auto& g : graphs) {
        check(graph_for(g.hash) == &g && contract(g, g.frame, g.constants, g.words), "exact graph accepted");
        check(!contract(g, g.frame + 1, g.constants, g.words), "changed frame rejected");
        check(!contract(g, g.frame, g.constants + 1, g.words), "changed constants rejected");
        check(!contract(g, g.frame, g.constants, g.words + 1), "changed code size rejected");
    }
    check(!graph_for(0), "unrelated graph ignored");
    Fixture f; Binding b{};
    check(bind_schema(f, f.objects, b), "all four exact embedded schemas bind");
    check(b.line_data.offset == 8 && b.reason.offset == 40 && b.line_score.offset == 4, "metadata offsets retained");
    for (unsigned i = 0; i < 4; ++i) {
        auto bad = f; bad.put(bad.records[i], type_hashes[i] ^ 1);
        check(!bind_schema(bad, bad.objects, b), "wrong native type hash rejected");
        bad = f; bad.put(bad.records[i] + 4, std::uint16_t{3 << 5});
        check(!bind_schema(bad, bad.objects, b), "pointer schema cannot masquerade as embedded struct");
        bad = f; bad.put(bad.records[i] + 0x2a, std::uint16_t{97});
        check(!bind_schema(bad, bad.objects, b), "reflection count is bounded");
        bad = f; bad.put(bad.tables[i] + 8, std::uint16_t{64});
        check(!bind_schema(bad, bad.objects, b), "out-of-bounds field rejected");
        bad = f; bad.put(bad.tables[i], std::uint32_t{0});
        check(!bind_schema(bad, bad.objects, b), "missing required field rejected");
    }
    auto bad = f; bad.put(bad.tables[0] + 16, bad.objects[3]);
    check(!bind_schema(bad, bad.objects, b), "embedded payload must match exact expected type");
    bad = f; std::uintptr_t primitive{}; bad(bad.boolean, primitive);
    bad.put(primitive + 4, std::uint16_t{15 << 5});
    check(!bind_schema(bad, bad.objects, b), "success requires bool kind as well as width");
    bad = f; bad(bad.number, primitive); bad.put(primitive + 6, std::uint16_t{8});
    check(!bind_schema(bad, bad.objects, b), "numeric field width checked");
    check(bind_schema(f, f.objects, b), "fresh valid binding remains available after rejected candidates");
    bad = f; bad.put(bad.objects[0], bad.records[1]);
    check(!type_matches(bad, b.types[0], type_hashes[0]), "retired type object fails record identity guard");
    Payload p{};
    f.put(f.payload + 12, std::int32_t{840}); f.put(f.payload + 40, std::uint32_t{7});
    check(decode(f, b, Route::line_end, f.payload, p) && p.score == 840 && p.reason == 7,
          "line score and reason copied without bank or bail policy");
    check(decode(f, b, Route::stats_line_end, f.payload, p) && p.score == 840, "second listener remains an explicit separate route");
    f.put(f.payload + 40, std::uint32_t{9}); p.score = 99;
    check(!decode(f, b, Route::line_end, f.payload, p) && p.score == 99, "unknown reason rejected without partial output");
    f.put(f.payload + 40, std::uint32_t{0}); f.put(f.payload + 12, std::int32_t{-1});
    check(!decode(f, b, Route::line_end, f.payload, p), "negative signed score rejected");
    f.put(f.payload + 12, std::int32_t{0});
    check(decode(f, b, Route::line_end, f.payload, p) && p.score == 0, "native zero is valid");
    b.line_score.kind = 19;
    for (float value : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -1.f, 2147483648.f}) {
        f.put(f.payload + 12, value); check(!decode(f, b, Route::line_end, f.payload, p), "nonfinite or out-of-range float score rejected");
    }
    f.put(f.payload + 12, 123.5f);
    check(decode(f, b, Route::line_end, f.payload, p) && p.score == 123.5, "fractional native score is not rounded or multiplied");
    f.put(f.payload + 12, std::int32_t{18518}); f.put(f.payload + 40, std::uint8_t{1});
    check(decode(f, b, Route::sequence_end, f.payload, p) && p.score == 18518 && p.success, "native sequence success and final score captured");
    f.put(f.payload + 40, std::uint8_t{0});
    check(decode(f, b, Route::sequence_end, f.payload, p) && !p.success, "unsuccessful native sequence is still evidence");
    f.put(f.payload + 40, std::uint8_t{2});
    check(!decode(f, b, Route::sequence_end, f.payload, p), "invalid success byte rejected");
    f.put(f.payload + 40, std::uint8_t{1}); b.sequence_score.kind = 16;
    f.put(f.payload + 12, UINT32_MAX);
    check(!decode(f, b, Route::sequence_end, f.payload, p), "unsigned score exceeds supported native range");
    f.put(f.payload + 12, std::uint32_t{2147483647});
    check(decode(f, b, Route::sequence_end, f.payload, p) && p.score == 2147483647., "largest supported native score copied exactly");
    check(!decode(f, b, Route::line_start, f.payload, p), "line start has no fabricated numeric payload");
    check(!decode(f, b, Route::sequence_end, 0, p), "unavailable payload fails closed");
    Buffer<2> queue; Event row{}; row.owner_verified = true;
    check(queue.push(row) && queue.push(row) && !queue.push(row) && queue.dropped() == 1, "full bounded buffer reports loss without overwriting");
    std::array<Event, 1> out{};
    check(queue.take(out) == 1 && out[0].serial == 1 && !out[0].owner_verified, "capture serial is ordered and ownership always unverified");
    check(queue.push(row) && queue.take(out) == 1 && out[0].serial == 2, "draining preserves oldest row across wraparound");
    check(queue.take(out) == 1 && out[0].serial == 3 && queue.take(out) == 0, "queue drains exactly once");
    Buffer<128> concurrent;
    std::atomic<unsigned> done{}, accepted{}, drained{}, corrupt{};
    std::array<std::thread, 4> writers;
    for (unsigned writer = 0; writer < writers.size(); ++writer) writers[writer] = std::thread([&, writer] {
        for (unsigned i = 1; i <= 10000; ++i) {
            Event e{}; e.epoch = writer * 10000 + i; e.sampled_at = e.epoch; e.payload.score = static_cast<double>(e.epoch);
            if (concurrent.push(e)) ++accepted;
        }
        ++done;
    });
    std::thread reader([&] {
        std::array<Event, 16> rows{}; std::uint64_t previous{};
        while (true) {
            const bool finished = done.load() == writers.size();
            const auto n = concurrent.take(rows);
            for (std::size_t i = 0; i < n; ++i) {
                const auto& e = rows[i];
                if (e.owner_verified || e.epoch != e.sampled_at || e.payload.score != e.epoch || e.serial <= previous) ++corrupt;
                previous = e.serial;
            }
            drained += static_cast<unsigned>(n);
            if (finished && !n) break;
            std::this_thread::yield();
        }
    });
    for (auto& writer : writers) writer.join(); reader.join();
    check(!corrupt && drained == accepted && accepted + concurrent.dropped() == 40000,
          "concurrent publication has no torn rows, missing accepted rows, or unreported loss");
    std::cout << checks << " native score checks passed\n";
}
