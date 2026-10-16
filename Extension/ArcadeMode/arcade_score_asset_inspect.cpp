#include "arcade_score_asset_inspect.h"
#include <algorithm>
#include <cctype>
#include <limits>

namespace dingosdk::arcade_mode::score_assets {
namespace {
namespace ebx = frostbite::ebx;
bool metadata_name(std::string_view name) noexcept {
    return name.size() <= 256 && std::all_of(name.begin(), name.end(), [](unsigned char c) { return c >= 32 && c < 127; });
}
int compare_names(std::string_view a, std::string_view b) noexcept {
    for (std::size_t i = 0; i < std::min(a.size(), b.size()); ++i) {
        const int left = std::tolower(static_cast<unsigned char>(a[i]));
        const int right = std::tolower(static_cast<unsigned char>(b[i]));
        if (left != right) return left < right ? -1 : 1;
    }
    return a.size() == b.size() ? 0 : a.size() < b.size() ? -1 : 1;
}
Json name_or_hash(std::string_view name, std::uint32_t hash) {
    Json result{{"hash", hash}, {"name", nullptr}};
    if (!name.empty() && metadata_name(name)) result["name"] = name;
    return result;
}
bool class_reference(const ebx::FieldDescriptor& f) noexcept {
    return f.type() == ebx::FieldType::inherited || f.type() == ebx::FieldType::structure ||
        f.type() == ebx::FieldType::pointer;
}
struct Walker {
    const ebx::Document& document;
    Json references = Json::array();
    std::array<const ebx::Object*, depth_limit + 1> ancestors{};
    std::size_t visited{}, omitted{};
    bool incomplete{};

    bool budget(unsigned depth) {
        if (depth > depth_limit || visited >= visit_limit || references.size() >= reference_limit) {
            incomplete = true; return false;
        }
        ++visited; return true;
    }
    void value(const ebx::Value& value, std::size_t instance, std::size_t field, unsigned depth, Json path) {
        if (!budget(depth)) return;
        const auto declared = document.fields[field].type();
        Json row{{"instance", instance}, {"field", field}, {"path", path}};
        if (const auto* pointer = std::get_if<ebx::PointerReference>(&value.data)) {
            if (declared != ebx::FieldType::pointer) { incomplete = true; return; }
            if (pointer->kind == ebx::PointerKind::null) row["kind"] = "null";
            else if (pointer->kind == ebx::PointerKind::internal && pointer->index >= 0 &&
                     static_cast<std::size_t>(pointer->index) < document.instances.size()) {
                row["kind"] = "instance"; row["target"] = pointer->index;
            } else if (pointer->kind == ebx::PointerKind::external && pointer->index >= 0 &&
                       static_cast<std::size_t>(pointer->index) < document.imports.size()) {
                const auto& import = document.imports[static_cast<std::size_t>(pointer->index)];
                row["kind"] = "import"; row["target"] = pointer->index;
                row["file_guid"] = import.fileGuid.string(); row["class_guid"] = import.classGuid.string();
            } else { incomplete = true; return; }
        } else if (const auto* resource = std::get_if<ebx::ResourceReference>(&value.data)) {
            if (declared != ebx::FieldType::resourceRef) { incomplete = true; return; }
            row["kind"] = "resource"; row["resource_id"] = resource->id;
        } else if (const auto* type = std::get_if<ebx::TypeReference>(&value.data)) {
            if (declared != ebx::FieldType::typeRef ||
                static_cast<unsigned>(type->primitiveType) > static_cast<unsigned>(ebx::FieldType::delegate)) {
                incomplete = true; return;
            }
            row["kind"] = "type";
            if (type->primitive) row["primitive"] = static_cast<unsigned>(type->primitiveType);
            else if (type->descriptor >= 0 && static_cast<std::size_t>(type->descriptor) < document.types.size())
                row["target"] = type->descriptor;
            else { incomplete = true; return; }
        } else if (std::holds_alternative<ebx::BoxedReference>(value.data)) {
            // cat pouch: opaque boxes may contain graph links; never guess or print their bytes.
            ++omitted; incomplete = true; return;
        } else if (const auto* object = std::get_if<std::shared_ptr<ebx::Object>>(&value.data)) {
            if (declared != ebx::FieldType::structure && declared != ebx::FieldType::inherited) {
                incomplete = true; return;
            }
            if (*object) object_value(**object, instance, depth + 1, path);
            else incomplete = true;
            return;
        } else if (const auto* array = std::get_if<ebx::Value::Array>(&value.data)) {
            if (document.fields[field].category() != ebx::FieldCategory::array ||
                (depth >= depth_limit && !array->empty())) { incomplete = true; return; }
            for (std::size_t i = 0; i < array->size(); ++i) {
                if (visited >= visit_limit || references.size() >= reference_limit) { incomplete = true; break; }
                auto child = path; child.push_back(Json{{"element", i}});
                this->value((*array)[i], instance, field, depth + 1, std::move(child));
            }
            return;
        } else return; // No string, score, setting, arbitrary number, GUID value, or raw image.
        references.push_back(std::move(row));
    }
    void object_value(const ebx::Object& object, std::size_t instance, unsigned depth, Json path) {
        if (!budget(depth)) return;
        if (object.descriptor < 0 || static_cast<std::size_t>(object.descriptor) >= document.types.size() ||
            object.fields.size() > field_limit) { incomplete = true; return; }
        for (unsigned i = 0; i < depth; ++i)
            if (ancestors[i] == &object) { incomplete = true; return; }
        ancestors[depth] = &object;
        for (const auto& member : object.fields) {
            if (visited >= visit_limit || references.size() >= reference_limit) { incomplete = true; break; }
            if (member.descriptor >= document.fields.size()) { incomplete = true; continue; }
            auto child = path; child.push_back(Json{{"field", member.descriptor}});
            value(member.value, instance, member.descriptor, depth + 1, std::move(child));
        }
        ancestors[depth] = nullptr;
    }
};
void declared_member(const ebx::Document& document, const ebx::Object& object, std::size_t instance,
                     Json& members, bool& incomplete) {
    if (object.descriptor < 0 || static_cast<std::size_t>(object.descriptor) >= document.types.size()) return;
    const auto& owner = document.types[object.descriptor];
    if (owner.name != "TypeInfoFieldData") return;
    if (members.size() >= declared_member_limit) { incomplete = true; return; }
    Json row{{"instance", instance}, {"owner_type", object.descriptor},
             {"owner_type_name", "TypeInfoFieldData"}, {"name", nullptr}, {"name_hash", nullptr}};
    bool name_seen{}, hash_seen{}, invalid_name{}, invalid_hash{};
    if (owner.fieldIndex < 0 || static_cast<std::size_t>(owner.fieldIndex) > document.fields.size() ||
        owner.fieldCount > document.fields.size() - static_cast<std::size_t>(owner.fieldIndex) ||
        object.fields.size() > field_limit) { incomplete = true; return; }
    for (const auto& member : object.fields) {
        // Only fields declared directly on the exact metadata type qualify.
        if (member.descriptor < static_cast<std::size_t>(owner.fieldIndex) ||
            member.descriptor - static_cast<std::size_t>(owner.fieldIndex) >= owner.fieldCount) continue;
        const auto& field = document.fields[member.descriptor];
        if (field.name == "Name") {
            if (name_seen) { invalid_name = true; continue; }
            name_seen = true;
            const auto* name = std::get_if<std::string>(&member.value.data);
            const auto initial = [](unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; };
            if (field.type() != ebx::FieldType::cString || field.category() != ebx::FieldCategory::none ||
                !name || name->empty() || name->size() > 128 || !initial(static_cast<unsigned char>((*name)[0])) ||
                !std::all_of(name->begin(), name->end(), [&](unsigned char c) { return initial(c) || (c >= '0' && c <= '9'); })) {
                invalid_name = true; continue;
            }
            row["name"] = *name;
        } else if (field.name == "NameHash") {
            if (hash_seen) { invalid_hash = true; continue; }
            hash_seen = true;
            const auto* hash = std::get_if<std::uint64_t>(&member.value.data);
            if (field.type() != ebx::FieldType::uint32 || field.category() != ebx::FieldCategory::none ||
                !hash || *hash > std::numeric_limits<std::uint32_t>::max()) { invalid_hash = true; continue; }
            row["name_hash"] = *hash;
        }
    }
    if (invalid_name) row["name"] = nullptr;
    if (invalid_hash) row["name_hash"] = nullptr;
    const bool valid = name_seen && hash_seen && !invalid_name && !invalid_hash;
    row["status"] = valid ? "DECLARED_MEMBER" : "INCOMPLETE";
    incomplete = incomplete || !valid;
    members.push_back(std::move(row));
}
void declared_parameter(const ebx::Document& document, const ebx::Object& object, std::size_t instance,
                        Json& parameters, bool& incomplete) {
    if (object.descriptor < 0 || static_cast<std::size_t>(object.descriptor) >= document.types.size()) return;
    const auto& owner = document.types[object.descriptor];
    if (owner.name != "TypeInfoParameterDataContainer") return;
    if (parameters.size() >= declared_member_limit || owner.fieldIndex < 0 ||
        static_cast<std::size_t>(owner.fieldIndex) > document.fields.size() ||
        owner.fieldCount > document.fields.size() - static_cast<std::size_t>(owner.fieldIndex) ||
        object.fields.size() > field_limit) { incomplete = true; return; }
    Json row{{"instance", instance}, {"owner_type", object.descriptor},
        {"name", nullptr}, {"qualifier", nullptr}, {"is_array", nullptr}};
    std::array<unsigned, 3> seen{};
    for (const auto& member : object.fields) {
        if (member.descriptor < static_cast<std::size_t>(owner.fieldIndex) ||
            member.descriptor - static_cast<std::size_t>(owner.fieldIndex) >= owner.fieldCount) continue;
        const auto& field = document.fields[member.descriptor];
        const int index = field.name == "Name" ? 0 : field.name == "Qualifier" ? 1 : field.name == "IsArray" ? 2 : -1;
        if (index < 0) continue;
        constexpr std::array keys{"name", "qualifier", "is_array"};
        if (++seen[index] != 1) { row[keys[index]] = nullptr; continue; }
        if (field.category() != ebx::FieldCategory::none) continue;
        if (index == 0 && field.type() == ebx::FieldType::cString) {
            const auto* value = std::get_if<std::string>(&member.value.data);
            const auto initial = [](unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; };
            if (value && !value->empty() && value->size() <= 128 && initial((*value)[0]) &&
                std::all_of(value->begin(), value->end(), [&](unsigned char c) { return initial(c) || (c >= '0' && c <= '9'); }))
                row["name"] = *value;
        } else if (index == 1 && field.type() == ebx::FieldType::enumeration) {
            const auto* value = std::get_if<std::int64_t>(&member.value.data);
            if (value && *value >= 0 && *value <= 3) row["qualifier"] = *value;
        } else if (index == 2 && field.type() == ebx::FieldType::boolean) {
            if (const auto* value = std::get_if<bool>(&member.value.data)) row["is_array"] = *value;
        }
    }
    const bool valid = seen == std::array<unsigned, 3>{1, 1, 1} && !row["name"].is_null() &&
        !row["qualifier"].is_null() && !row["is_array"].is_null();
    row["status"] = valid ? "DECLARED_PARAMETER" : "INCOMPLETE";
    incomplete = incomplete || !valid; parameters.push_back(std::move(row));
}
}
bool allowed_asset(std::string_view name) noexcept {
    if (!metadata_name(name) || name.empty() || name.find('\\') != name.npos || name.find("..") != name.npos) return false;
    constexpr std::array prefixes{"ecs/scoringmanager/", "ecs/legacyscoringmanager/"};
    for (std::string_view prefix : prefixes) {
        if (name.size() <= prefix.size()) continue;
        bool same = true;
        for (std::size_t i = 0; i < prefix.size(); ++i)
            same = same && static_cast<char>(std::tolower(static_cast<unsigned char>(name[i]))) == prefix[i];
        if (same) return true;
    }
    return false;
}
unsigned asset_priority(std::string_view name) noexcept {
    if (!allowed_asset(name)) return 4;
    const auto leaf = name.substr(name.find_last_of('/') + 1);
    constexpr std::array graph_names{
        "ScoringTelemetryManager_Class__OnScorableLineStarted",
        "ScoringTelemetryManager_Class__OnScorableLineEnded",
        "StatsManager__OnScorableLineEnded",
        "ScoringTelemetryManager__OnScorableSequenceEnded"};
    for (std::string_view target : graph_names)
        if (compare_names(leaf, target) == 0) return 0;
    // Exact manifest identity from the pinned asset inspection. Selecting this
    // candidate does not assign it the older compiled graph hash contract.
    if (compare_names(name, "ecs/scoringmanager/scoringtelemetrymanager_class__onscorablesequenceended") == 0) return 0;
    // These exact candidates appeared in the bounded manifest inventory. Their
    // parsed GUIDs still determine whether an imported declaration is resolved.
    for (std::string_view target : {"ecs/scoringmanager/scoringtelemetrymanager_class",
                                   "ecs/scoringmanager/activescoringdatamodel",
                                   "ecs/scoringmanager/linedata",
                                   "ecs/scoringmanager/scoringdataresource",
                                   "ecs/scoringmanager/scoringtelemetrymanager_class_ecsschematics",
                                   "ecs/scoringmanager/scoringtelemetrymanager_class_schematics"})
        if (compare_names(name, target) == 0) return 1;
    constexpr std::string_view global = "ecs/scoringmanager/crosssystem_global_scoringevents/";
    if (name.size() > global.size() && compare_names(name.substr(0, global.size()), global) == 0) {
        constexpr std::array events{"scorablelinestarted", "scorablelineended", "scorablesequencestarted",
            "scorablesequenceended", "sequenceactionended", "scorablelinemultiplierincreased",
            "scoringsequenceregisteredinline"};
        for (std::string_view event : events)
            if (compare_names(leaf, event) == 0) return 2;
    }
    return 3;
}
bool matches_declaration(const ebx::Document& document, const DeclarationTarget& target) {
    if (document.fileGuid.string() != target.file_guid || document.instances.size() > instance_limit ||
        document.types.size() > type_limit) return false;
    return std::count_if(document.instances.begin(), document.instances.end(), [&](const auto& instance) {
        return instance.exported && instance.instanceGuid.string() == target.class_guid &&
            instance.descriptor >= 0 && static_cast<std::size_t>(instance.descriptor) < document.types.size() &&
            instance.object && instance.object->descriptor == instance.descriptor;
    }) == 1;
}
void DeclarationInventory::consider(AssetCandidate candidate) noexcept {
    if (candidate.resource || !allowed_asset(candidate.name)) return;
    const auto less = [](const auto& a, const auto& b) {
        // Keep root declaration candidates ahead of generated handler names;
        // neither naming shape nor inventory inclusion proves a declaration.
        const bool a_handler = a.name.find("__") != std::string_view::npos;
        const bool b_handler = b.name.find("__") != std::string_view::npos;
        if (a_handler != b_handler) return !a_handler;
        const auto compared = compare_names(a.name, b.name);
        return compared ? compared < 0 : a.bundle != b.bundle ? a.bundle < b.bundle : a.index < b.index;
    };
    std::size_t position{};
    while (position < size && !less(candidate, assets[position])) ++position;
    if (size == assets.size()) omitted = true;
    if (position == assets.size()) return;
    if (size < assets.size()) ++size;
    for (auto i = size - 1; i > position; --i) assets[i] = assets[i - 1];
    assets[position] = candidate;
}
void AssetSelection::consider(AssetCandidate candidate) noexcept {
    if (!allowed_asset(candidate.name)) return;
    const auto less = [](const AssetCandidate& a, const AssetCandidate& b) {
        const auto ap = asset_priority(a.name), bp = asset_priority(b.name);
        if (ap != bp) return ap < bp;
        if (a.resource != b.resource) return !a.resource;
        const int compared = compare_names(a.name, b.name);
        if (compared) return compared < 0;
        if (a.bundle != b.bundle) return a.bundle < b.bundle;
        return a.index < b.index;
    };
    std::size_t position{};
    while (position < size && !less(candidate, assets[position])) ++position;
    if (size == asset_limit) omitted = true;
    if (position == asset_limit) return;
    if (size < asset_limit) ++size;
    for (std::size_t i = size - 1; i > position; --i) assets[i] = assets[i - 1];
    assets[position] = candidate;
}
Json targets() {
    return Json::array({
        Json{{"graph_hash", 0xa4d1121cU}, {"name", "ScoringTelemetryManager_Class__OnScorableLineStarted"}},
        Json{{"graph_hash", 0x69843dadU}, {"name", "ScoringTelemetryManager_Class__OnScorableLineEnded"}},
        Json{{"graph_hash", 0xe2d57480U}, {"name", "StatsManager__OnScorableLineEnded"}},
        Json{{"graph_hash", 0xafef12e9U}, {"name", "ScoringTelemetryManager__OnScorableSequenceEnded"}}});
}
Json inspect_document(const ebx::Document& document) {
    Json result{{"status", "INCOMPLETE"}, {"owner_proof", "UNVERIFIED"}, {"graph_producer", "UNVERIFIED"},
                {"file_guid", document.fileGuid.string()}, {"types", Json::array()}, {"fields", Json::array()},
                {"instances", Json::array()}, {"references", Json::array()}, {"declared_members", Json::array()},
                {"declared_parameters", Json::array()}};
    if (document.types.size() > type_limit || document.fields.size() > field_limit ||
        document.instances.size() > instance_limit || document.imports.size() > reference_limit) {
        result["reason"] = "declaration_limit"; return result;
    }
    bool incomplete{};
    for (std::size_t i = 0; i < document.types.size(); ++i) {
        const auto& type = document.types[i];
        if (type.fieldIndex < 0 || static_cast<std::size_t>(type.fieldIndex) > document.fields.size() ||
            type.fieldCount > document.fields.size() - static_cast<std::size_t>(type.fieldIndex)) {
            incomplete = true; continue;
        }
        Json row = name_or_hash(type.name, type.nameHash);
        row["index"] = i; row["kind"] = static_cast<unsigned>(type.type());
        row["field_start"] = type.fieldIndex; row["field_count"] = type.fieldCount;
        result["types"].push_back(std::move(row));
    }
    for (std::size_t i = 0; i < document.fields.size(); ++i) {
        const auto& field = document.fields[i];
        Json row = name_or_hash(field.name, field.nameHash);
        row["index"] = i; row["kind"] = static_cast<unsigned>(field.type());
        row["category"] = static_cast<unsigned>(field.category());
        row["declared_type"] = nullptr;
        if (class_reference(field)) {
            if (field.classRef < document.types.size()) row["declared_type"] = field.classRef;
            else incomplete = true;
        }
        result["fields"].push_back(std::move(row));
    }
    Walker walker{document};
    for (std::size_t i = 0; i < document.instances.size(); ++i) {
        const auto& instance = document.instances[i];
        if (instance.descriptor < 0 || static_cast<std::size_t>(instance.descriptor) >= document.types.size()) {
            incomplete = true; continue;
        }
        result["instances"].push_back(Json{{"index", i}, {"type", instance.descriptor},
            {"exported", instance.exported}, {"instance_guid", instance.instanceGuid.string()}});
        if (instance.object) {
            if (instance.object->descriptor != instance.descriptor) incomplete = true;
            else {
                declared_member(document, *instance.object, i, result["declared_members"], incomplete);
                declared_parameter(document, *instance.object, i, result["declared_parameters"], incomplete);
            }
            walker.object_value(*instance.object, i, 0, Json::array());
        }
        else incomplete = true;
    }
    result["references"] = std::move(walker.references);
    result["visited"] = walker.visited; result["omitted_boxed_references"] = walker.omitted;
    result["status"] = incomplete || walker.incomplete ? "INCOMPLETE" : "DECLARATIONS_READ";
    return result;
}
}
