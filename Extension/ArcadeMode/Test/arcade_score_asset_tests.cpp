#include "Extension/ArcadeMode/arcade_score_asset_inspect.h"
#include <cstdlib>
#include <iostream>

namespace {
namespace scoring = dingosdk::arcade_mode::score_assets;
namespace ebx = dingosdk::frostbite::ebx;
using dingosdk::Json;
unsigned checks{};
void check(bool ok, const char* label) {
    ++checks; if (!ok) { std::cerr << "FAIL: " << label << '\n'; std::exit(1); }
}
std::uint16_t encoded(ebx::FieldType type, ebx::FieldCategory category = ebx::FieldCategory::none) {
    return static_cast<std::uint16_t>((static_cast<unsigned>(type) << 4) | static_cast<unsigned>(category));
}
ebx::Document fixture() {
    ebx::Document document;
    ebx::TypeDescriptor type; type.name = "ScoringTelemetryManager"; type.nameHash = 12;
    type.fieldCount = 6; type.encodedType = encoded(ebx::FieldType::structure); type.size = 64;
    document.types.push_back(type);
    document.fields = {
        {"Context", 20, encoded(ebx::FieldType::pointer), 0, 0},
        {"Expression", 21, encoded(ebx::FieldType::resourceRef), 0, 8},
        {"DeclaredType", 22, encoded(ebx::FieldType::typeRef), 0, 16},
        {"Description", 23, encoded(ebx::FieldType::cString), 0, 24},
        {"Value", 24, encoded(ebx::FieldType::uint32), 0, 32},
        {"Nested", 25, encoded(ebx::FieldType::structure), 0, 40}};
    auto object = std::make_shared<ebx::Object>(); object->descriptor = 0;
    object->fields = {
        {0, "ignored_value_name", ebx::Value{ebx::PointerReference{ebx::PointerKind::external, 0}}},
        {1, "ignored_value_name", ebx::Value{ebx::ResourceReference{12345}}},
        {2, "ignored_value_name", ebx::Value{ebx::TypeReference{false, {}, 0, 0}}},
        {3, "ignored_value_name", ebx::Value{std::string{"private-player-value"}}},
        {4, "ignored_value_name", ebx::Value{std::uint64_t{987654321}}}};
    ebx::InstanceRecord instance; instance.descriptor = 0; instance.object = object;
    instance.rawImage = {std::byte{0x7f}, std::byte{0xee}};
    document.instances.push_back(instance); document.imports.emplace_back();
    return document;
}
ebx::Object& root(ebx::Document& document) { return *document.instances[0].object; }
}
int main() {
    check(scoring::allowed_asset("ecs/scoringmanager/ScoringTelemetryManager"), "scoring manager asset selected");
    check(scoring::allowed_asset("ECS/LegacyScoringManager/Manager"), "known prefix is case insensitive");
    for (std::string_view name : {"ecs/scoringmanager/", "ecs/scoringmanagerx/value", "../ecs/scoringmanager/value",
                                 "ecs/scoringmanager/../../private", "ecs/scoringmanager/value\nprivate",
                                 "ecs/scoringmanager/\\private", "gameplay/scorables/flip", "/ecs/scoringmanager/value"})
        check(!scoring::allowed_asset(name), "outside fixed scoring scope rejected");
    check(!scoring::allowed_asset(std::string("ecs/scoringmanager/") + std::string(257, 'a')), "asset name bounded");
    auto document = fixture(); auto output = scoring::inspect_document(document);
    check(output["status"].string() == "DECLARATIONS_READ", "valid declarations read");
    check(output["owner_proof"].string() == "UNVERIFIED" && output["graph_producer"].string() == "UNVERIFIED",
          "context-named reference cannot establish ownership");
    check(output["references"].size() == 3 && output["references"][0]["kind"].string() == "import" &&
          output["references"][1]["resource_id"].get<unsigned>() == 12345, "only typed reference identities exported");
    check(output.dump().find("private-player-value") == std::string::npos &&
          output.dump().find("987654321") == std::string::npos &&
          output.dump().find("ignored_value_name") == std::string::npos, "scalar, string, and value-side names omitted");
    for (unsigned invalid = 0; invalid < 6; ++invalid) {
        document = fixture();
        if (invalid == 0) document.types[0].fieldIndex = -1;
        if (invalid == 1) document.types[0].fieldCount = 7;
        if (invalid == 2) document.fields[0].classRef = 99;
        if (invalid == 3) document.instances[0].descriptor = 99;
        if (invalid == 4) root(document).fields[0].descriptor = 99;
        if (invalid == 5) root(document).descriptor = 99;
        check(scoring::inspect_document(document)["status"].string() == "INCOMPLETE", "malformed declaration indices explicit");
    }
    for (unsigned invalid = 0; invalid < 4; ++invalid) {
        document = fixture();
        root(document).fields[0].value = ebx::Value{ebx::PointerReference{
            invalid < 2 ? ebx::PointerKind::internal : ebx::PointerKind::external, invalid % 2 ? 99 : -1}};
        output = scoring::inspect_document(document);
        check(output["status"].string() == "INCOMPLETE" && output["references"].size() == 2,
              "invalid reference targets omitted without inventing context association");
    }
    document = fixture(); document.fields[0].encodedType = encoded(ebx::FieldType::uint32);
    check(scoring::inspect_document(document)["status"].string() == "INCOMPLETE", "reference variant requires matching declaration");
    document = fixture(); document.fields[3].name = "unsafe\nname";
    output = scoring::inspect_document(document);
    check(output["fields"][3]["name"].is_null() && output["fields"][3]["hash"].get<unsigned>() == 23,
          "unprintable declaration uses hash only");
    for (unsigned limit = 0; limit < 4; ++limit) {
        document = fixture();
        if (limit == 0) document.types.resize(scoring::type_limit + 1);
        if (limit == 1) document.fields.resize(scoring::field_limit + 1);
        if (limit == 2) document.instances.resize(scoring::instance_limit + 1);
        if (limit == 3) document.imports.resize(scoring::reference_limit + 1);
        output = scoring::inspect_document(document);
        check(output["status"].string() == "INCOMPLETE" && output["references"].empty(), "document limits stop before traversal");
    }
    document = fixture();
    document.fields[0].encodedType = encoded(ebx::FieldType::pointer, ebx::FieldCategory::array);
    ebx::Value::Array array(scoring::reference_limit + 1, ebx::Value{ebx::PointerReference{ebx::PointerKind::internal, 0}});
    root(document).fields[0].value = ebx::Value{array};
    output = scoring::inspect_document(document);
    check(output["status"].string() == "INCOMPLETE" && output["references"].size() == scoring::reference_limit,
          "large reference array stops at fixed record budget");
    document = fixture();
    document.fields[4].encodedType = encoded(ebx::FieldType::uint32, ebx::FieldCategory::array);
    root(document).fields[4].value = ebx::Value{ebx::Value::Array(scoring::visit_limit + 1, ebx::Value{std::uint64_t{1}})};
    output = scoring::inspect_document(document);
    check(output["status"].string() == "INCOMPLETE" && output["visited"].get<unsigned>() == scoring::visit_limit,
          "ignored scalar arrays still consume bounded traversal budget");
    document = fixture();
    auto shared = document.instances[0].object;
    root(document).fields.push_back({5, {}, ebx::Value{shared}});
    output = scoring::inspect_document(document);
    check(output["status"].string() == "INCOMPLETE" && output["visited"].get<unsigned>() < 20,
          "object cycle stops without recursive expansion");
    root(document).fields.pop_back(); // Break the synthetic shared_ptr cycle.
    document = fixture();
    ebx::Value nested{ebx::PointerReference{ebx::PointerKind::internal, 0}};
    for (unsigned i = 0; i < 20; ++i) nested = ebx::Value{ebx::Value::Array{nested}};
    document.fields[0].encodedType = encoded(ebx::FieldType::pointer, ebx::FieldCategory::array);
    root(document).fields[0].value = nested;
    output = scoring::inspect_document(document);
    check(output["status"].string() == "INCOMPLETE" && output["visited"].get<unsigned>() < 30,
          "nested arrays stop at depth limit");
    document = fixture(); root(document).fields[0].value = ebx::Value{ebx::BoxedReference{}};
    output = scoring::inspect_document(document);
    check(output["status"].string() == "INCOMPLETE" && output["omitted_boxed_references"].get<unsigned>() == 1,
          "opaque boxed references explicitly incomplete without raw bytes");
    check(scoring::targets().size() == 4 && scoring::targets()[0]["graph_hash"].get<std::uint32_t>() == 0xa4d1121c,
          "only four pinned graph targets advertised");
    std::cout << checks << " scoring asset metadata checks passed\n";
}
