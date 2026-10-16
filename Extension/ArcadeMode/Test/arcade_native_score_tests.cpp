#include "Extension/ArcadeMode/arcade_native_score_buffer.h"
#include "Extension/ArcadeMode/arcade_score_boundary.h"
#include "Extension/ArcadeMode/arcade_score_schema_snapshot.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <thread>
#include <fstream>
#include <sstream>
#ifdef ARCADE_CAPTURE_HEALTH_COMBINED_TESTS
int arcade_score_capture_health_tests();
#endif

namespace {
using namespace dingosdk::arcade_mode::native_score_detail;
unsigned checks{};
void check(bool ok, const char* description) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}
void capture_scope_tests() {
    struct Scope { std::uintptr_t vm{}; const Scope* parent{}; };
    std::atomic<std::uint64_t> rejected{};
    check(capture_scope_allowed(static_cast<const Scope*>(nullptr), 1, rejected) && rejected == 0,
          "top-level score capture has no duplicate ancestor");
    std::array<Scope, 33> ancestors{};
    for (std::size_t i = 0; i < ancestors.size(); ++i)
        ancestors[i] = {2, i ? &ancestors[i - 1] : nullptr};
    check(capture_scope_allowed(&ancestors[31], 1, rejected) && rejected == 0,
          "exactly 32 distinct ancestors remain within existing bound");
    ancestors[0].vm = 1;
    check(!capture_scope_allowed(&ancestors[31], 1, rejected) && rejected == 0,
          "proven same-VM duplicate at last permitted ancestor does not count as loss");
    check(!capture_scope_allowed(&ancestors[32], 1, rejected) && rejected == 1,
          "same VM beyond bound cannot silently suppress an unverified capture");
    ancestors[0].vm = 2;
    check(!capture_scope_allowed(&ancestors[32], 1, rejected) && rejected == 2,
          "exhausted walk increments existing rejection counter exactly once");
    ancestors[32].vm = 1;
    check(!capture_scope_allowed(&ancestors[32], 1, rejected) && rejected == 2,
          "immediate duplicate suppresses without needing the remaining ancestry");
    ancestors[0].parent = &ancestors[0];
    check(!capture_scope_allowed(&ancestors[0], 1, rejected) && rejected == 3,
          "cyclic observer ancestry terminates and reports incomplete capture");
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
void schema_snapshot_tests() {
    Fixture f; Binding binding{};
    check(bind_schema(f, f.objects, binding), "schema snapshot fixture binds existing contracts");
    const auto active = [] { return true; };
    auto snapshot = snapshot_score_schema(f, binding, active);
    check(snapshot.status == SchemaSnapshotStatus::complete && snapshot.types[0].count == 1 &&
          snapshot.types[1].count == 1, "bounded schema snapshot captures both existing types");
    check(snapshot.types[0].members[0] == SchemaMember{0x40bff667, 123, 15, 4} &&
          snapshot.types[1].members[0] == SchemaMember{2099127164U, 123, 15, 4},
          "sanitized schema rows contain only expected hashes kind and size, no source addresses");
    unsigned calls{};
    snapshot = snapshot_score_schema(f, binding, [&] { return ++calls == 1; });
    check(snapshot.status == SchemaSnapshotStatus::inactive && snapshot.types[0].count == 0,
          "epoch expiry during snapshot discards every member");
    calls = 0;
    snapshot = snapshot_score_schema(f, binding, [&] {
        if (++calls == 2) f.put(f.records[1], std::uint32_t{999});
        return true;
    });
    check(snapshot.status == SchemaSnapshotStatus::type_changed && snapshot.types[0].count == 0,
          "type identity changed after reads cannot publish metadata");
    f.put(f.records[1], type_hashes[1]);
    f.put(f.records[1] + 0x2a, std::uint16_t{97});
    check(snapshot_score_schema(f, binding, active).status == SchemaSnapshotStatus::invalid_fields,
          "97 member table rejected before traversal");
    f.put(f.records[1] + 0x2a, std::uint16_t{0});
    check(snapshot_score_schema(f, binding, active).status == SchemaSnapshotStatus::invalid_fields,
          "empty table cannot replace already bound score fields");
    f.put(f.records[1] + 0x2a, std::uint16_t{1});
    f.put(f.tables[1] + 8, std::uint16_t{15});
    check(snapshot_score_schema(f, binding, active).status == SchemaSnapshotStatus::invalid_fields,
          "out-of-type field extent rejected by existing field reader");
    f.put(f.tables[1] + 8, std::uint16_t{4});
    f.put(f.tables[1] + 16, std::uintptr_t{0});
    check(snapshot_score_schema(f, binding, active).status == SchemaSnapshotStatus::invalid_fields,
          "unreadable referenced type cannot produce a row");
    f.put(f.tables[1] + 16, f.number);
    f.put(f.records[1] + 0x2a, std::uint16_t{2});
    f.add(1, 1, 0x40bff667, 8, f.number);
    check(snapshot_score_schema(f, binding, active).status == SchemaSnapshotStatus::invalid_fields,
          "duplicate field hashes rejected rather than silently resolving first field twice");
    f.put(f.records[1] + 0x2a, std::uint16_t{1});
    unsigned table_reads{};
    auto changing = [&](std::uintptr_t address, auto& value) {
        if (address == f.tables[1] && ++table_reads == 3) f.put(f.tables[1], std::uint32_t{42});
        return f(address, value);
    };
    check(snapshot_score_schema(changing, binding, active).status == SchemaSnapshotStatus::fields_changed,
          "different bounded declaration passes cannot publish a coherent-looking snapshot");
    f.put(f.tables[1], std::uint32_t{0x40bff667});
    const auto large = f.alloc(96 * 24);
    f.tables[1] = large; f.put(f.records[1] + 0x60, large); f.put(f.records[1] + 0x2a, std::uint16_t{96});
    for (unsigned i = 0; i < 96; ++i) f.add(1, i, i + 1, 0, f.number);
    snapshot = snapshot_score_schema(f, binding, active);
    check(snapshot.status == SchemaSnapshotStatus::complete && snapshot.types[0].count == 96 &&
          snapshot.types[0].members[95].hash == 96, "exact 96 member boundary retained");
    SchemaSnapshotGate gate;
    auto slot = gate.begin(binding);
    check(slot == 0, "first stable type set gets a bounded report slot");
    gate.finish(slot, SchemaSnapshotStatus::complete);
    check(gate.begin(binding) == SchemaSnapshotGate::denied,
          "same type set is not repeated after a reset or epoch change");
    SchemaSnapshotGate retries;
    for (unsigned i = 0; i < 3; ++i) {
        slot = retries.begin(binding); check(slot == 0, "incomplete schema attempt has bounded retry");
        retries.finish(slot, SchemaSnapshotStatus::inactive);
    }
    check(retries.begin(binding) == SchemaSnapshotGate::denied, "fourth incomplete retry is suppressed");
    for (unsigned i = 1; i < 8; ++i) {
        binding.types[1].object += 8;
        slot = gate.begin(binding); check(slot == i, "new type identity gets separate slot");
        gate.finish(slot, SchemaSnapshotStatus::complete);
    }
    binding.types[1].object += 8;
    check(gate.begin(binding) == SchemaSnapshotGate::denied, "lifetime type-set budget cannot grow unbounded");
}
struct DiagnosticFixture : Fixture {
    std::uintptr_t enum_type{}, enum_record{}, float_type{}, float_record{};
    std::uintptr_t action_type{}, action_record{}, parent_type{}, parent_record{}, category_table{}, action{};
    DiagnosticFixture() {
        put(records[2] + 6, std::uint16_t{128}); put(records[3] + 6, std::uint16_t{64});
        add(2, 1, 3611278405U, 112, boolean);
        tables[3] = alloc(6 * 24); put(records[3] + 0x60, tables[3]);
        put(records[3] + 0x2a, std::uint16_t{6});
        enum_type = type(125, 8, 4, enum_record); float_type = type(126, 19, 4, float_record);
        action_type = type(127, 3, 32, action_record); parent_type = type(128, 3, 16, parent_record);
        put(action_record + 0x30, parent_type);
        category_table = alloc(24); put(parent_record + 0x38, category_table); put(parent_record + 0x2a, std::uint16_t{1});
        put(category_table, std::uint32_t{0x26e74827}); put(category_table + 8, std::uint16_t{8});
        put(category_table + 16, enum_type);
        add(3, 0, 2099127164U, 4, number); add(3, 1, 2702987617U, 0, enum_type);
        add(3, 2, 3836815661U, 8, number); add(3, 3, 3691294820U, 12, number);
        add(3, 4, 2088787037U, 16, float_type); add(3, 5, 333924837U, 24, action_type);
        payload = alloc(128); action = alloc(32);
        put(payload + 112, std::uint8_t{1}); put(payload + 8, std::uint32_t{1});
        put(payload + 12, std::int32_t{90}); put(payload + 16, std::int32_t{60});
        put(payload + 20, std::int32_t{40}); put(payload + 24, .75f);
        put(payload + 32, action); put(action + 8, std::uint32_t{13});
    }
};
struct HintRead : Fixture {
    unsigned reads{};
    std::uintptr_t replace_at{}, replacement{};
    template<class T> bool operator()(std::uintptr_t at, T& value) noexcept {
        ++reads;
        if (replacement && at == replace_at) {
            put(objects[0], replacement); replacement = 0;
        }
        return Fixture::operator()(at, value);
    }
};
void hint_tests() {
    Fixture original; Binding previous{}, next{};
    check(bind_schema(original, original.objects, previous), "hint starts from a validated schema");
    check(rebind_schema_hint(original, previous, next) && next.line_score.offset == 4,
          "unchanged type hints can rebuild the complete schema");
    auto moved = original; moved.put(moved.tables[1] + 8, std::uint16_t{8});
    check(rebind_schema_hint(moved, previous, next) && next.line_score.offset == 8 && previous.line_score.offset == 4,
          "valid changed field offset is rebound instead of reused from the prior epoch");
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned fault = 0; fault < 7; ++fault) {
            auto bad = original;
            if (fault == 0) bad.put(bad.records[i], type_hashes[i] ^ 1);
            if (fault == 1) bad.put(bad.records[i] + 4, std::uint16_t{3 << 5});
            if (fault == 2) bad.put(bad.records[i] + 6, std::uint16_t{128});
            if (fault == 3) bad.put(bad.objects[i], std::uintptr_t{0x90000});
            if (fault == 4) bad.put(bad.tables[i] + 8, std::uint16_t{64});
            if (fault == 5) bad.put(bad.tables[i], std::uint32_t{});
            if (fault == 6) bad.put(bad.records[i] + 0x2a, std::uint16_t{97});
            next = previous; next.reason.offset = 123;
            check(!rebind_schema_hint(bad, previous, next) && next.reason.offset == 123,
                  "stale type or invalid full schema rejects hints without partial publication");
        }
        auto changed = original;
        const auto replacement = changed.alloc(0x68);
        std::memcpy(changed.bytes.data() + replacement - 0x10000,
                    changed.bytes.data() + changed.records[i] - 0x10000, 0x68);
        changed.put(changed.objects[i], replacement);
        check(!rebind_schema_hint(changed, previous, next),
              "replacement record with the same hash still requires discovery");
        check(bind_schema(changed, changed.objects, next) && next.types[i].record == replacement,
              "normal discovery binder remains usable after a rejected record hint");
    }
    check(!rebind_schema_hint(original, Binding{}, next), "cold start has no usable hints");
    auto wrong_field = original; wrong_field.put(wrong_field.tables[0] + 16, wrong_field.objects[3]);
    check(!rebind_schema_hint(wrong_field, previous, next), "hints retain exact embedded payload association checks");
    DiagnosticFixture diagnostics;
    check(bind_schema(diagnostics, diagnostics.objects, previous) && previous.sequence.scalars && previous.sequence.category_depth,
          "diagnostic hint starts with complete optional schema");
    diagnostics.put(diagnostics.tables[3] + 24, std::uint32_t{});
    diagnostics.put(diagnostics.tables[3] + 5 * 24, std::uint32_t{});
    check(rebind_schema_hint(diagnostics, previous, next) && !next.sequence.scalars && !next.sequence.category_depth,
          "retired optional declarations are not carried across the lifecycle epoch");
    HintRead counted;
    check(bind_schema(counted, counted.objects, previous), "bounded read fixture initialized");
    counted.reads = 0;
    check(rebind_schema_hint(counted, previous, next) && counted.reads < 200,
          "minimal schema hint uses bounded metadata reads without an address-space scan");
    counted.replacement = counted.alloc(0x68);
    std::memcpy(counted.bytes.data() + counted.replacement - 0x10000,
                counted.bytes.data() + counted.records[0] - 0x10000, 0x68);
    counted.replace_at = counted.tables[0]; next.reason.offset = 123;
    check(!rebind_schema_hint(counted, previous, next) && next.reason.offset == 123,
          "record replacement during field binding fails final hint validation without publishing");
}
void diagnostic_tests() {
    DiagnosticFixture f; Binding b{};
    check(bind_schema(f, f.objects, b) && b.sequence.scalars && b.sequence.category_depth == 2,
          "optional scalar and inherited category fields bind from reflected declarations");
    auto d = decode_sequence_diagnostics(f, b, f.payload);
    check(d.scalars == DiagnosticStatus::available && d.kind == 1 && d.action_score == 60 &&
          d.base_score == 40 && d.landing == .75f, "different native score fields retained without arithmetic");
    check(d.category == DiagnosticStatus::available && d.current_category == 13,
          "declared category read through bounded class ancestry");
    Payload p{};
    check(decode(f, b, Route::sequence_end, f.payload, p) && p.score == 90 && p.success,
          "sequence total remains separate from optional action and base values");
    f.put(f.payload + 32, std::uintptr_t{});
    check(decode_sequence_diagnostics(f, b, f.payload).category == DiagnosticStatus::absent,
          "null current action is explicitly absent, not category zero");
    f.put(f.payload + 32, std::uintptr_t{0x90000});
    check(decode_sequence_diagnostics(f, b, f.payload).category == DiagnosticStatus::read_failed,
          "unreadable action reference is distinct from absent");
    f.put(f.payload + 32, f.action); f.put(f.action + 8, UINT32_MAX);
    check(decode_sequence_diagnostics(f, b, f.payload).current_category == UINT32_MAX,
          "unrecognized enum observation never becomes a fabricated trick eligibility rule");
    for (float invalid : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
        f.put(f.payload + 24, invalid); d = decode_sequence_diagnostics(f, b, f.payload);
        check(d.scalars == DiagnosticStatus::invalid_value && d.action_score == 0 && d.base_score == 0 &&
              d.category == DiagnosticStatus::available, "nonfinite landing invalidates scalars without partial numeric output");
        check(decode(f, b, Route::sequence_end, f.payload, p) && p.score == 90,
              "diagnostic failures preserve original sequence capture");
    }
    f = DiagnosticFixture{}; f.put(f.payload + 16, std::int32_t{-1});
    check(decode_sequence_diagnostics(f, b, f.payload).scalars == DiagnosticStatus::invalid_value,
          "negative signed action score is not reinterpreted as unsigned");
    f = DiagnosticFixture{}; f.put(f.payload + 20, std::int32_t{-1});
    check(decode_sequence_diagnostics(f, b, f.payload).scalars == DiagnosticStatus::invalid_value,
          "negative signed base score rejected");
    f = DiagnosticFixture{}; auto unsigned_binding = b; unsigned_binding.sequence.action_score.kind = 16;
    f.put(f.payload + 16, UINT32_MAX);
    check(decode_sequence_diagnostics(f, unsigned_binding, f.payload).scalars == DiagnosticStatus::invalid_value,
          "unsigned action score outside supported range remains invalid");
    f.put(f.payload + 16, std::uint32_t{2147483647}); f.put(f.payload + 24, -2.5f);
    d = decode_sequence_diagnostics(f, unsigned_binding, f.payload);
    check(d.scalars == DiagnosticStatus::available && d.action_score == 2147483647. && d.landing == -2.5f,
          "exact unsigned maximum and finite landing retained without assumed landing semantics");
    f = DiagnosticFixture{};
    check(decode_sequence_diagnostics(f, b, 0x90000).scalars == DiagnosticStatus::read_failed,
          "unavailable scalar data has explicit read failure");
    f.put(f.action_type, f.parent_record);
    check(decode_sequence_diagnostics(f, b, f.payload).category == DiagnosticStatus::schema_unavailable,
          "retired declared action type is not read with cached category offset");
    f = DiagnosticFixture{}; f.put(f.action_record + 0x30, std::uintptr_t{});
    check(decode_sequence_diagnostics(f, b, f.payload).category == DiagnosticStatus::schema_unavailable,
          "changed parent chain invalidates category binding");
    for (unsigned field_index = 1; field_index <= 5; ++field_index) {
        f = DiagnosticFixture{}; f.put(f.tables[3] + field_index * 24, std::uint32_t{});
        check(bind_schema(f, f.objects, b), "missing optional field never disables original score observer");
        d = decode_sequence_diagnostics(f, b, f.payload);
        check((field_index == 5 ? d.category : d.scalars) == DiagnosticStatus::schema_unavailable,
              "missing diagnostic declaration never supplies numeric evidence");
    }
    for (unsigned invalid = 0; invalid < 6; ++invalid) {
        f = DiagnosticFixture{};
        if (invalid == 0) f.put(f.enum_record + 4, std::uint16_t{15 << 5});
        if (invalid == 1) f.put(f.enum_record + 6, std::uint16_t{8});
        if (invalid == 2) f.put(f.float_record + 4, std::uint16_t{15 << 5});
        if (invalid == 3) f.put(f.float_record + 6, std::uint16_t{8});
        if (invalid == 4) f.put(f.tables[3] + 2 * 24 + 16, f.float_type);
        if (invalid == 5) f.put(f.tables[3] + 3 * 24 + 8, std::uint16_t{63});
        check(bind_schema(f, f.objects, b) && !b.sequence.scalars,
              "scalar diagnostics require exact enum, integer and float kinds, widths and bounds");
    }
    for (unsigned invalid = 0; invalid < 6; ++invalid) {
        f = DiagnosticFixture{};
        if (invalid == 0) f.put(f.parent_record + 0x2a, std::uint16_t{97});
        if (invalid == 1) { f.put(f.category_table, std::uint32_t{}); f.put(f.parent_record + 0x30, f.action_type); }
        if (invalid == 2) f.put(f.category_table + 8, std::uint16_t{15});
        if (invalid == 3) f.put(f.category_table + 16, f.number);
        if (invalid == 4) f.put(f.parent_record + 0x38, std::uintptr_t{0x90000});
        if (invalid == 5) f.put(f.tables[3] + 5 * 24 + 8, std::uint16_t{60});
        check(bind_schema(f, f.objects, b) && !b.sequence.category_depth && b.sequence.scalars,
              "bad category metadata, cycles and reference bounds fail independently of scalars");
    }
    f = DiagnosticFixture{};
    auto previous = f.action_record;
    for (unsigned depth = 0; depth < 8; ++depth) {
        std::uintptr_t record{}; auto object = f.type(200 + depth, 3, 32, record);
        f.put(previous + 0x30, object); previous = record;
    }
    f.put(previous + 0x2a, std::uint16_t{1}); f.put(previous + 0x38, f.category_table);
    check(bind_schema(f, f.objects, b) && !b.sequence.category_depth,
          "category beyond eight declared class levels cannot expand traversal");
    f = DiagnosticFixture{}; check(bind_schema(f, f.objects, b), "diagnostic fixture restored");
    Buffer<1> queue; Event row{}; row.owner_verified = true;
    row.payload = {90, 0, true}; row.sequence = decode_sequence_diagnostics(f, b, f.payload);
    row.epoch = 7; row.sampled_at = 1000; row.route = Route::sequence_end;
    std::array<Event, 1> out{};
    check(queue.push(row) && queue.take(out) == 1 && out[0].sequence == row.sequence && !out[0].owner_verified,
          "optional diagnostics survive queue publication without enabling ownership");
    check(dingosdk::arcade_mode::native_score_capture_status(out[0], 7, 1000, true) ==
          dingosdk::arcade_mode::ScoreDecision::ambiguous, "complete diagnostics never enable monetary authority");
    dingosdk::arcade_mode::ScoreBoundary gate;
    dingosdk::arcade_mode::ScoreContext context{11, 7, 3, 1000, 0, 0, 0, 0, true, true};
    gate.begin(context, 0);
    check(gate.observe(out[0], {}, context).decision == dingosdk::arcade_mode::ScoreDecision::ambiguous,
          "scalar diagnostics cannot supply missing scoring proof");
    out[0].sequence.base_score += 1;
    check(gate.observe(out[0], {}, context).decision == dingosdk::arcade_mode::ScoreDecision::stale,
          "same capture serial with changed diagnostics is not an exact transport duplicate");
}
}
int main(int argc, char** argv) {
    capture_scope_tests();
    schema_snapshot_tests();
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
#ifdef ARCADE_CAPTURE_HEALTH_COMBINED_TESTS
    check(arcade_score_capture_health_tests() == 0, "capture health regression checks");
#endif
    hint_tests();
    diagnostic_tests();
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
