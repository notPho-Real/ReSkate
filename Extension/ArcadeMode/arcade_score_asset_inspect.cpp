#include "arcade_score_asset_inspect.h"
#include <algorithm>
#include <cctype>

namespace dingosdk::arcade_mode::score_assets {
namespace {
namespace ebx = frostbite::ebx;
bool metadata_name(std::string_view name) noexcept {
    return name.size() <= 256 && std::all_of(name.begin(), name.end(), [](unsigned char c) { return c >= 32 && c < 127; });
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
                {"instances", Json::array()}, {"references", Json::array()}};
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
        if (instance.object) walker.object_value(*instance.object, i, 0, Json::array());
        else incomplete = true;
    }
    result["references"] = std::move(walker.references);
    result["visited"] = walker.visited; result["omitted_boxed_references"] = walker.omitted;
    result["status"] = incomplete || walker.incomplete ? "INCOMPLETE" : "DECLARATIONS_READ";
    return result;
}
}
