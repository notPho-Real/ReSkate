#include "ebx_type_import.h"
#include "ebx_carry.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <stdexcept>

namespace dingosdk::frostbite::ebx {
namespace {
struct Tail {
    std::vector<std::byte> groups_and_mappings;
    std::map<std::uint32_t, std::string> names;
};
Tail reflection_tail(const Document& document) {
    if (!document.hashedReflectionNames) throw std::runtime_error("Type import needs RFL2 reflection");
    BinaryReader r(document.reflectionData);
    const auto signatures = r.u32();
    if (signatures != document.types.size()) throw std::runtime_error("Incomplete type signatures");
    (void)r.view(static_cast<std::size_t>(signatures) * 20);
    if (r.u32() != document.types.size()) throw std::runtime_error("Reflection type count differs");
    (void)r.view(document.types.size() * 16);
    if (r.u32() != document.fields.size()) throw std::runtime_error("Reflection field count differs");
    (void)r.view(document.fields.size() * 12);
    const auto begin = r.position();
    const auto groups = r.u32();
    (void)r.view(static_cast<std::size_t>(groups) * 12);
    const auto mappings = r.u32();
    (void)r.view(static_cast<std::size_t>(mappings) * 8);
    Tail result;
    result.groups_and_mappings.assign(document.reflectionData.begin() + begin,
                                      document.reflectionData.begin() + r.position());
    const auto names = r.u32();
    std::map<std::uint32_t, std::uint32_t> offsets;
    for (std::uint32_t i = 0; i < names; ++i) {
        const auto hash = r.u32(), offset = r.u32();
        if (!offsets.emplace(hash, offset).second) throw std::runtime_error("Duplicate reflection name");
    }
    const auto strings = r.position();
    for (const auto& [hash, offset] : offsets) {
        if (offset >= r.size() - strings) throw std::runtime_error("Invalid reflection name offset");
        r.seek(strings + offset);
        result.names.emplace(hash, r.c_string());
    }
    return result;
}
void add_name(Tail& tail, std::uint32_t hash, const std::string& name) {
    const auto [it, inserted] = tail.names.emplace(hash, name);
    if (!inserted && it->second != name) throw std::runtime_error("Reflection name hash collision");
}
bool has_class(const FieldDescriptor& field) {
    return field.type() == FieldType::inherited || field.type() == FieldType::structure ||
           field.type() == FieldType::enumeration;
}
void rebuild_reflection(Document& document, const Tail& tail) {
    BinaryWriter w;
    w.u32(static_cast<std::uint32_t>(document.types.size()));
    for (const auto& type : document.types) { w.guid(type.guid); w.u32(type.signature); }
    w.u32(static_cast<std::uint32_t>(document.types.size()));
    for (const auto& type : document.types) {
        w.u32(type.nameHash); w.i32(type.fieldIndex); w.u16(type.fieldCount);
        w.u16(static_cast<std::uint16_t>(type.encodedType << 1)); w.u16(type.size); w.u16(type.alignment);
    }
    w.u32(static_cast<std::uint32_t>(document.fields.size()));
    for (const auto& field : document.fields) {
        w.u32(field.nameHash); w.u32(field.dataOffset);
        w.u16(static_cast<std::uint16_t>(field.encodedType << 1)); w.u16(field.classRef);
    }
    w.bytes(tail.groups_and_mappings);
    w.u32(static_cast<std::uint32_t>(tail.names.size()));
    BinaryWriter strings;
    for (const auto& [hash, name] : tail.names) {
        if (strings.position() > UINT32_MAX) throw std::runtime_error("Reflection names exceed their width");
        w.u32(hash); w.u32(static_cast<std::uint32_t>(strings.position()));
        for (const auto ch : name) strings.u8(static_cast<std::uint8_t>(ch));
        strings.u8(0);
    }
    w.bytes(strings.data());
    document.reflectionData = w.take();
}
}
Document with_imported_type(const Document& target, const Document& donor, std::string_view type_name) {
    auto tail = reflection_tail(target);
    // Validate the donor table before trusting its parsed descriptors.
    (void)reflection_tail(donor);
    auto result = detail::clone_document(target);
    std::map<std::string, std::size_t, std::less<>> known;
    for (std::size_t i = 0; i < result.types.size(); ++i)
        if (!known.emplace(result.types[i].name, i).second) throw std::runtime_error("Duplicate type name");
    const auto selected = std::ranges::find(donor.types, type_name, &TypeDescriptor::name);
    if (selected == donor.types.end()) throw std::runtime_error("Donor type is missing");
    std::map<std::size_t, std::size_t> imported;
    std::function<std::size_t(std::size_t, unsigned)> carry = [&](std::size_t index, unsigned depth) -> std::size_t {
        if (depth > 128 || index >= donor.types.size()) throw std::runtime_error("Invalid type dependency");
        if (const auto it = imported.find(index); it != imported.end()) return it->second;
        const auto source = donor.types[index];
        if (source.fieldIndex < 0 || static_cast<std::size_t>(source.fieldIndex) > donor.fields.size() ||
            source.fieldCount > donor.fields.size() - static_cast<std::size_t>(source.fieldIndex))
            throw std::runtime_error("Invalid donor fields");
        if (const auto it = known.find(source.name); it != known.end()) {
            const auto& existing = result.types[it->second];
            if (existing.guid != source.guid || existing.signature != source.signature ||
                existing.size != source.size || existing.alignment != source.alignment ||
                existing.encodedType != source.encodedType || existing.fieldCount != source.fieldCount)
                throw std::runtime_error("Conflicting donor type: " + source.name);
            for (std::size_t i = 0; i < source.fieldCount; ++i) {
                const auto& a = donor.fields.at(static_cast<std::size_t>(source.fieldIndex) + i);
                const auto& b = result.fields.at(static_cast<std::size_t>(existing.fieldIndex) + i);
                if (a.name != b.name || a.nameHash != b.nameHash || a.dataOffset != b.dataOffset ||
                    a.encodedType != b.encodedType || (source.type() != FieldType::enumeration && has_class(a) &&
                    donor.types.at(a.classRef).name != result.types.at(b.classRef).name))
                    throw std::runtime_error("Conflicting donor field: " + source.name + "." + a.name);
            }
            imported.emplace(index, it->second);
            return it->second;
        }
        if (result.types.size() >= UINT16_MAX || result.fields.size() > INT32_MAX - source.fieldCount)
            throw std::runtime_error("Imported schema exceeds descriptor widths");
        auto type = source;
        type.fieldIndex = static_cast<std::int32_t>(result.fields.size());
        const auto destination = result.types.size();
        result.types.push_back(type);
        known.emplace(type.name, destination);
        imported.emplace(index, destination);
        add_name(tail, type.nameHash, type.name);
        // Reserve all fields first: a dependency may append fields of its own.
        result.fields.insert(result.fields.end(), donor.fields.begin() + source.fieldIndex,
                             donor.fields.begin() + source.fieldIndex + source.fieldCount);
        for (std::size_t i = 0; i < source.fieldCount; ++i) {
            auto field = donor.fields[static_cast<std::size_t>(source.fieldIndex) + i];
            if (field.category() == FieldCategory::array || field.category() == FieldCategory::function ||
                field.type() == FieldType::boxedValueRef || field.type() == FieldType::typeRef)
                throw std::runtime_error("Donor type needs unsupported reflection mappings");
            // Enumeration members use the same encoding as inheritance, but
            // their offsets are values and classRef is not a dependency.
            if (source.type() != FieldType::enumeration && has_class(field))
                field.classRef = static_cast<std::uint16_t>(carry(field.classRef, depth + 1));
            add_name(tail, field.nameHash, field.name);
            result.fields[static_cast<std::size_t>(type.fieldIndex) + i] = std::move(field);
        }
        return destination;
    };
    carry(static_cast<std::size_t>(selected - donor.types.begin()), 0);
    if (result.types.size() != target.types.size()) rebuild_reflection(result, tail);
    return result;
}
}
