#include "Extension/ArcadeMode/arcade_score_asset_inspect.h"
#include <cstdlib>
#include <iostream>
#include <algorithm>

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
        {2, "ignored_value_name", ebx::Value{ebx::TypeReference{false, {}, 1, 4}}},
        {3, "ignored_value_name", ebx::Value{std::string{"private-player-value"}}},
        {4, "ignored_value_name", ebx::Value{std::uint64_t{987654321}}}};
    ebx::InstanceRecord instance; instance.descriptor = 0; instance.object = object;
    instance.rawImage = {std::byte{0x7f}, std::byte{0xee}};
    document.instances.push_back(instance); document.imports.emplace_back();
    return document;
}
ebx::Object& root(ebx::Document& document) { return *document.instances[0].object; }
ebx::Document member_fixture() {
    ebx::Document document;
    ebx::TypeDescriptor type; type.name = "TypeInfoFieldData"; type.fieldCount = 3;
    type.encodedType = encoded(ebx::FieldType::structure); document.types.push_back(type);
    document.fields = {{"Name", 1, encoded(ebx::FieldType::cString), 0, 0},
                       {"NameHash", 2, encoded(ebx::FieldType::uint32), 0, 8},
                       {"Description", 3, encoded(ebx::FieldType::cString), 0, 16}};
    auto object = std::make_shared<ebx::Object>(); object->descriptor = 0;
    object->fields = {{0, {}, ebx::Value{std::string{"ScorableLine"}}},
                      {1, {}, ebx::Value{std::uint64_t{123}}},
                      {2, {}, ebx::Value{std::string{"private unrelated string"}}}};
    ebx::InstanceRecord instance; instance.descriptor = 0; instance.object = object;
    document.instances.push_back(instance); return document;
}
ebx::Document parameter_fixture() {
    auto document = member_fixture();
    document.types[0].name = "TypeInfoParameterDataContainer";
    document.types[0].fieldCount = 4;
    document.fields[1] = {"Qualifier", 2, encoded(ebx::FieldType::enumeration), 0, 8};
    document.fields.push_back({"IsArray", 4, encoded(ebx::FieldType::boolean), 0, 24});
    root(document).fields[0].value = ebx::Value{std::string{"Value"}};
    root(document).fields[1].value = ebx::Value{std::int64_t{2}};
    root(document).fields.push_back({3, {}, ebx::Value{false}});
    return document;
}
}
int main() {
    check(scoring::allowed_asset("ecs/scoringmanager/ScoringTelemetryManager"), "scoring manager asset selected");
    check(scoring::allowed_asset("ECS/LegacyScoringManager/Manager"), "known prefix is case insensitive");
    for (std::string_view name : {"ecs/scoringmanager/", "ecs/scoringmanagerx/value", "../ecs/scoringmanager/value",
                                 "ecs/scoringmanager/../../private", "ecs/scoringmanager/value\nprivate",
                                 "ecs/scoringmanager/\\private", "gameplay/scorables/flip", "/ecs/scoringmanager/value"})
        check(!scoring::allowed_asset(name), "outside fixed scoring scope rejected");
    check(!scoring::allowed_asset(std::string("ecs/scoringmanager/") + std::string(257, 'a')), "asset name bounded");
    std::vector<std::string> names;
    for (unsigned i = 0; i < 80; ++i) names.push_back("ecs/scoringmanager/aaa_other_" + std::to_string(i));
    for (const auto& target : scoring::targets()) names.push_back("ecs/scoringmanager/" + target["name"].string());
    names.push_back("ecs/scoringmanager/crosssystem_global_scoringevents/scorablelineended");
    names.push_back("ecs/scoringmanager/crosssystem_global_scoringevents/scorablesequenceended");
    std::vector<scoring::AssetCandidate> candidates;
    for (std::size_t i = 0; i < names.size(); ++i) candidates.push_back({names[i], i < 80 ? 0U : 1U, i, false});
    scoring::AssetSelection selected, reversed;
    for (auto candidate : candidates) selected.consider(candidate);
    std::reverse(candidates.begin(), candidates.end());
    for (auto candidate : candidates) reversed.consider(candidate);
    check(selected.size == scoring::asset_limit && selected.omitted, "32 asset cap retains explicit omitted status");
    check(std::all_of(selected.assets.begin(), selected.assets.begin() + 4, [](const auto& asset) {
        return scoring::asset_priority(asset.name) == 0 && asset.bundle == 1;
    }), "all four late second-bundle graph declarations displace first-bundle prefix matches");
    check(scoring::asset_priority(selected.assets[4].name) == 2 && scoring::asset_priority(selected.assets[5].name) == 2,
          "global line and sequence declarations precede broad prefix matches");
    check(std::equal(selected.assets.begin(), selected.assets.end(), reversed.assets.begin(), [](const auto& a, const auto& b) {
        return a.name == b.name && a.bundle == b.bundle && a.index == b.index && a.resource == b.resource;
    }), "bounded selection deterministic under reversed manifest traversal");
    scoring::AssetSelection invalid_selection;
    invalid_selection.consider({"other/scoringtelemetrymanager_class__onscorablelinestarted", 0, 0, false});
    check(invalid_selection.size == 0 && !invalid_selection.omitted, "target basename cannot escape existing namespaces");
    check(scoring::asset_priority("ecs/scoringmanager/scoringtelemetrymanager_class__onscorablelinestarted_extra") == 3,
          "target matching exact rather than guessed name prefix");
    scoring::AssetSelection kinds;
    kinds.consider({names[80], 1, 0, true}); kinds.consider({names[80], 1, 1, false});
    check(!kinds.assets[0].resource && kinds.assets[1].resource, "graph EBX declaration precedes resource identity");
    selected.consider({"ecs/scoringmanager/scoringtelemetrymanager_class", 1, 1000, false});
    selected.consider({"ecs/scoringmanager/activescoringdatamodel", 1, 1001, false});
    check(scoring::asset_priority(selected.assets[4].name) == 1 && scoring::asset_priority(selected.assets[5].name) == 1 &&
          selected.size == scoring::asset_limit, "context declaration candidates displace filler without increasing cap");
    constexpr std::array observed_candidates{
        "ecs/scoringmanager/linedata", "ecs/scoringmanager/scoringdataresource",
        "ecs/scoringmanager/scoringtelemetrymanager_class_ecsschematics",
        "ecs/scoringmanager/scoringtelemetrymanager_class_schematics",
        "ecs/scoringmanager/scoringtelemetrymanager_class__onscorablesequenceended"};
    for (const auto name : observed_candidates) selected.consider({name, 1, 1002, false});
    check(selected.size == scoring::asset_limit && selected.omitted, "observed candidates preserve original asset cap");
    for (const auto name : observed_candidates)
        check(std::any_of(selected.assets.begin(), selected.assets.end(), [&](const auto& candidate) {
            return candidate.name == name && !candidate.resource;
        }), "actual context and sequence handler candidates displace unrelated filler");
    check(scoring::asset_priority("ECS/SCORINGMANAGER/SCORINGTELEMETRYMANAGER_CLASS__ONSCORABLESEQUENCEENDED") == 0,
          "actual sequence handler identity is case insensitive");
    check(scoring::asset_priority("ecs/scoringmanager/linedata_extra") == 3 &&
          scoring::asset_priority("ecs/scoringmanager/unrelated/linedata") == 3 &&
          scoring::asset_priority("ecs/scoringmanager/scoringtelemetrymanager_class__onscorablesequenceended_extra") == 3,
          "observed candidate paths exact, without broadened prefixes or guessed variants");
    check(scoring::targets()[3]["graph_hash"].get<std::uint32_t>() == 0xafef12e9 &&
          scoring::targets()[3]["name"].string() == "ScoringTelemetryManager__OnScorableSequenceEnded",
          "manifest candidate selection does not rewrite compiled runtime graph identity");
    std::vector<std::string> inventory_names;
    for (unsigned i = 0; i < scoring::manifest_name_limit + 8; ++i)
        inventory_names.push_back("ecs/scoringmanager/declaration_" + std::to_string(i));
    scoring::DeclarationInventory inventory, inventory_reverse;
    inventory.consider({"ecs/scoringmanager/resource", 0, 0, true});
    inventory.consider({"elsewhere/private", 0, 0, false});
    check(inventory.size == 0, "inventory never broadens namespace or includes resources");
    for (std::size_t i = 0; i < inventory_names.size(); ++i) inventory.consider({inventory_names[i], 0, i, false});
    for (std::size_t i = inventory_names.size(); i-- > 0;) inventory_reverse.consider({inventory_names[i], 0, i, false});
    check(inventory.size == scoring::manifest_name_limit && inventory.omitted, "manifest inventory bounded with explicit omission");
    check(std::equal(inventory.assets.begin(), inventory.assets.end(), inventory_reverse.assets.begin(),
        [](const auto& a, const auto& b) { return a.name == b.name; }), "inventory independent of manifest traversal order");
    inventory.consider({"ecs/scoringmanager/aaa__handler", 0, 999, false});
    check(inventory.assets[0].name == inventory_reverse.assets[0].name,
          "generated handler names do not displace root declaration candidates");
    auto document = fixture(); auto output = scoring::inspect_document(document);
    check(output["status"].string() == "DECLARATIONS_READ", "valid declarations read");
    check(output["owner_proof"].string() == "UNVERIFIED" && output["graph_producer"].string() == "UNVERIFIED",
          "context-named reference cannot establish ownership");
    check(output["references"].size() == 3 && output["references"][0]["kind"].string() == "import" &&
          output["references"][1]["resource_id"].get<unsigned>() == 12345, "only typed reference identities exported");
    check(output["references"][2]["serialized_index"].get<unsigned>() == 1 &&
          output["references"][2]["index_space"].string() == "UNRESOLVED_SERIALIZED_TYPE_REFERENCE" &&
          output["references"][2].dump().find("target") == std::string::npos,
          "serialized type slot outside local reflection never becomes a false target index");
    document = fixture();
    document.fixupTypeGuids.resize(2); document.fixupTypeGuids[0].bytes[0] = std::byte{1};
    document.fixupTypeSignatures = {123, 456}; document.types[0].signature = 456;
    output = scoring::inspect_document(document);
    check(output["fixup_types"][1]["reflection_index"].get<unsigned>() == 0 &&
          output["fixup_types"][0]["reflection_index"].is_null() &&
          output["type_reference_mapping"].string() == "UNVERIFIED",
          "EFIX identity mapping exported separately from raw serialized type slot");
    document.types.push_back(document.types[0]); output = scoring::inspect_document(document);
    check(output["fixup_types"][1]["status"].string() == "AMBIGUOUS" &&
          output["fixup_types"][1]["reflection_index"].is_null(), "duplicate reflection identity never selects first match");
    document.types.pop_back(); document.fixupTypeSignatures.pop_back(); output = scoring::inspect_document(document);
    check(output["fixup_types"][1]["status"].string() == "SIGNATURE_UNAVAILABLE" &&
          output["fixup_types"][1]["reflection_index"].is_null(), "GUID without signature does not identify local reflection");
    document.fixupTypeGuids.resize(scoring::type_limit + 1);
    check(scoring::inspect_document(document)["reason"].string() == "declaration_limit", "fixup metadata table shares type budget");
    document = fixture(); document.fixupTypeSignatures.push_back(1);
    check(scoring::inspect_document(document)["reason"].string() == "declaration_limit", "signature table cannot exceed GUID table");
    document = fixture(); root(document).fields[2].value = ebx::Value{ebx::TypeReference{}};
    check(scoring::inspect_document(document)["references"][2]["null"].get<bool>(), "null type reference retained explicitly");
    document = fixture(); root(document).fields[2].value = ebx::Value{ebx::TypeReference{false, {}, 2, 4}};
    check(scoring::inspect_document(document)["status"].string() == "INCOMPLETE", "inconsistent encoded type index rejected");
    document = fixture(); output = scoring::inspect_document(document);
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
    document = member_fixture(); output = scoring::inspect_document(document);
    check(output["declared_members"].size() == 1 && output["declared_members"][0]["name"].string() == "ScorableLine" &&
          output["declared_members"][0]["name_hash"].get<unsigned>() == 123 &&
          output["declared_members"][0]["owner_type_name"].string() == "TypeInfoFieldData",
          "exact declaration owner exports bounded member name and hash for existing reference join");
    check(output.dump().find("private unrelated string") == std::string::npos &&
          output["owner_proof"].string() == "UNVERIFIED", "unrelated declaration text omitted without owner inference");
    document.types[0].name = "UnrelatedType";
    check(scoring::inspect_document(document)["declared_members"].empty(), "Name on arbitrary object cannot export text");
    for (unsigned malformed = 0; malformed < 8; ++malformed) {
        document = member_fixture();
        if (malformed == 0) root(document).fields[0].value = ebx::Value{std::string(129, 'a')};
        if (malformed == 1) root(document).fields[0].value = ebx::Value{std::string{"arbitrary text with spaces"}};
        if (malformed == 2) document.fields[0].encodedType = encoded(ebx::FieldType::uint32);
        if (malformed == 3) root(document).fields[1].value = ebx::Value{std::uint64_t{0x100000000ULL}};
        if (malformed == 4) root(document).fields.erase(root(document).fields.begin() + 1);
        if (malformed == 5) root(document).fields.push_back(root(document).fields[0]);
        if (malformed == 6) document.types[0].fieldCount = 1;
        if (malformed == 7) root(document).fields[1].value = ebx::Value{std::string{"123"}};
        output = scoring::inspect_document(document);
        check(output["status"].string() == "INCOMPLETE" &&
              output["declared_members"][0]["status"].string() == "INCOMPLETE",
              "malformed or unbounded declaration identity explicitly incomplete");
    }
    document = member_fixture(); document.instances.resize(scoring::declared_member_limit + 1, document.instances[0]);
    output = scoring::inspect_document(document);
    check(output["declared_members"].size() == scoring::declared_member_limit && output["status"].string() == "INCOMPLETE",
          "declaration member cap stays bounded and incomplete");
    document = parameter_fixture(); output = scoring::inspect_document(document);
    check(output["declared_parameters"].size() == 1 && output["declared_parameters"][0]["name"].string() == "Value" &&
          output["declared_parameters"][0]["qualifier"].get<int>() == 2 &&
          !output["declared_parameters"][0]["is_array"].get<bool>(), "exact parameter metadata exported");
    check(output.dump().find("private unrelated string") == std::string::npos &&
          output["owner_proof"].string() == "UNVERIFIED", "parameter declaration never supplies owner proof or unrelated text");
    document.types[0].name = "OtherParameter";
    check(scoring::inspect_document(document)["declared_parameters"].empty(), "parameter type must match exactly");
    for (unsigned malformed = 0; malformed < 13; ++malformed) {
        document = parameter_fixture();
        if (malformed == 0) root(document).fields[0].value = ebx::Value{std::string(129, 'x')};
        if (malformed == 1) root(document).fields[0].value = ebx::Value{std::string{"unsafe text"}};
        if (malformed == 2) document.fields[0].encodedType = encoded(ebx::FieldType::uint32);
        if (malformed == 3) root(document).fields[1].value = ebx::Value{std::int64_t{-1}};
        if (malformed == 4) root(document).fields[1].value = ebx::Value{std::int64_t{4}};
        if (malformed == 5) root(document).fields[1].value = ebx::Value{std::uint64_t{2}};
        if (malformed == 6) document.fields[1].encodedType = encoded(ebx::FieldType::int32);
        if (malformed == 7) root(document).fields[3].value = ebx::Value{std::uint64_t{0}};
        if (malformed == 8) document.fields[3].encodedType = encoded(ebx::FieldType::boolean, ebx::FieldCategory::array);
        if (malformed == 9) root(document).fields.pop_back();
        if (malformed == 10) root(document).fields.push_back(root(document).fields[1]);
        if (malformed == 11) document.types[0].fieldCount = 1;
        if (malformed == 12) root(document).fields[0].value = ebx::Value{std::string{"9bad"}};
        output = scoring::inspect_document(document);
        check(output["status"].string() == "INCOMPLETE" &&
              output["declared_parameters"][0]["status"].string() == "INCOMPLETE", "malformed parameter rejected");
    }
    document = parameter_fixture(); document.instances.resize(scoring::declared_member_limit + 1, document.instances[0]);
    check(scoring::inspect_document(document)["declared_parameters"].size() == scoring::declared_member_limit,
          "parameter output cap retained");
    constexpr scoring::DeclarationTarget zero{"fixture", "00000000-0000-0000-0000-000000000000", "00000000-0000-0000-0000-000000000000"};
    document = fixture(); document.instances[0].exported = true;
    check(scoring::matches_declaration(document, zero), "actual document file and exported class identity resolve");
    document.instances.push_back(document.instances[0]);
    check(!scoring::matches_declaration(document, zero), "duplicate exported class identity cannot resolve an import");
    document.instances.pop_back();
    check(!scoring::matches_declaration(document, scoring::declaration_targets[0]), "unrelated selected document cannot resolve target");
    document.instances[0].exported = false;
    check(!scoring::matches_declaration(document, zero), "unexported instance cannot resolve imported declaration");
    document.instances[0].exported = true; document.instances[0].descriptor = 999;
    check(!scoring::matches_declaration(document, zero), "invalid descriptor cannot resolve declaration");
    document = fixture(); document.instances[0].exported = true; document.fileGuid.bytes[0] = std::byte{1};
    check(!scoring::matches_declaration(document, zero), "file identity must match independently of class identity");
    document.fileGuid.bytes[0] = std::byte{0}; document.instances[0].instanceGuid.bytes[0] = std::byte{1};
    check(!scoring::matches_declaration(document, zero), "class identity must match independently of file identity");
    std::cout << checks << " scoring asset metadata checks passed\n";
}
