#include "Engine/Resource/ebx_type_import.h"
#include "Engine/Resource/ebx_carry.h"
#include "Engine/Resource/ebx_writer.h"
#include <iostream>
#include <cstring>
#include <map>
#include <stdexcept>

namespace fb = dingosdk::frostbite;
namespace e = fb::ebx;
namespace {
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
std::uint32_t hash(std::string_view text) {
    std::uint32_t h = 5381;
    for (unsigned char c : text) h = h * 33 ^ c;
    return h;
}
// Entirely authored fixture; no game data is embedded in this test.
void reflection(e::Document& d) {
    fb::BinaryWriter w, strings;
    std::map<std::uint32_t, std::string> names;
    w.u32(static_cast<std::uint32_t>(d.types.size()));
    for (const auto& t : d.types) { w.guid(t.guid); w.u32(t.signature); names[t.nameHash] = t.name; }
    w.u32(static_cast<std::uint32_t>(d.types.size()));
    for (const auto& t : d.types) {
        w.u32(t.nameHash); w.i32(t.fieldIndex); w.u16(t.fieldCount);
        w.u16(static_cast<std::uint16_t>(t.encodedType << 1)); w.u16(t.size); w.u16(t.alignment);
    }
    w.u32(static_cast<std::uint32_t>(d.fields.size()));
    for (const auto& f : d.fields) {
        w.u32(f.nameHash); w.u32(f.dataOffset); w.u16(static_cast<std::uint16_t>(f.encodedType << 1));
        w.u16(f.classRef); names[f.nameHash] = f.name;
    }
    w.u32(0); w.u32(0);
    w.u32(static_cast<std::uint32_t>(names.size()));
    for (const auto& [h, name] : names) {
        w.u32(h); w.u32(static_cast<std::uint32_t>(strings.position()));
        for (unsigned char c : name) strings.u8(c);
        strings.u8(0);
    }
    w.bytes(strings.data()); d.reflectionData = w.take(); d.hashedReflectionNames = true;
}
void add(e::Document& d, const std::string& type_name, const std::string& field_name,
         e::FieldType field_type, e::Value value) {
    const auto index = static_cast<std::int32_t>(d.types.size());
    e::TypeDescriptor t;
    t.name = type_name; t.nameHash = hash(type_name); t.fieldIndex = static_cast<std::int32_t>(d.fields.size());
    t.fieldCount = 1; t.encodedType = 0x30; t.size = 32; t.alignment = 8;
    t.guid.bytes.fill(static_cast<std::byte>(index + 1)); t.signature = hash(type_name);
    d.types.push_back(t);
    e::FieldDescriptor f;
    f.name = field_name; f.nameHash = hash(field_name); f.dataOffset = 16;
    f.encodedType = static_cast<std::uint16_t>(static_cast<unsigned>(field_type) << 4); f.classRef = 0xffff;
    d.fields.push_back(f);
    e::InstanceRecord i;
    i.descriptor = index; i.fixupType = e::ensure_fixup_type(d, index);
    i.object = std::make_shared<e::Object>(); i.object->descriptor = index;
    i.object->fields.push_back({static_cast<std::size_t>(t.fieldIndex), field_name, std::move(value)});
    d.instances.push_back(std::move(i));
}
template<class F> void refuses(F&& f) {
    bool rejected = false;
    try { f(); } catch (const std::exception&) { rejected = true; }
    check(rejected, "unsupported import was accepted");
}
}
int main() try {
    e::Document target, donor;
    add(target, "Container", "Untouched", e::FieldType::uint32, e::Value{std::uint64_t{37}, {}});
    add(target, "OtherContainer", "KeepOther", e::FieldType::uint32, e::Value{std::uint64_t{17}, {}});
    add(donor, "Padding", "Unrelated", e::FieldType::uint32, e::Value{std::uint64_t{99}, {}});
    add(donor, "NativeLabel", "RawText", e::FieldType::cString, e::Value{std::string("12x"), {}});
    reflection(target); reflection(donor);
    const auto before = e::write_document(target, true);
    auto patched = e::with_imported_type(target, donor, "NativeLabel");
    e::detail::Carrier carrier(donor, patched, true);
    const auto index = carrier.instance(1);
    const auto decoded = e::read_document(e::write_document(patched, true));
    check(std::get<std::string>(decoded.instances.at(index).object->find("RawText")->value.data) == "12x",
          "carried label lost its text across field-table remapping");
    check(std::get<std::uint64_t>(decoded.instances.front().object->find("Untouched")->value.data) == 37,
          "existing data changed");
    check(e::write_document(target, true) == before && target.types.size() == 2, "import mutated its input");
    auto wrong = e::with_imported_type(target, donor, "NativeLabel");
    e::detail::Carrier old_carrier(donor, wrong);
    const auto wrong_index = old_carrier.instance(1);
    const auto wrong_decoded = e::read_document(e::write_document(wrong, true));
    check(std::get<std::string>(wrong_decoded.instances.at(wrong_index).object->find("RawText")->value.data) != "12x",
          "remapping regression control did not distinguish the old behavior");
    const auto repeated = e::with_imported_type(patched, donor, "NativeLabel");
    check(repeated.types.size() == patched.types.size() && repeated.reflectionData == patched.reflectionData,
          "reimport duplicated the schema");
    auto conflict = e::detail::clone_document(donor);
    ++conflict.types[1].signature; reflection(conflict);
    refuses([&] { (void)e::with_imported_type(patched, conflict, "NativeLabel"); });
    auto array = e::detail::clone_document(donor);
    array.fields[1].encodedType |= static_cast<std::uint16_t>(e::FieldCategory::array); reflection(array);
    refuses([&] { (void)e::with_imported_type(target, array, "NativeLabel"); });
    refuses([&] { (void)e::with_imported_type(target, donor, "Missing"); });
    auto truncated = donor; truncated.reflectionData.resize(4);
    refuses([&] { (void)e::with_imported_type(target, truncated, "NativeLabel"); });
    e::Value values{e::Value::Array{e::Value{std::uint64_t{2}, {}}}, 0x12345678};
    auto copied = e::detail::clone_value(values);
    check(copied.arrayHash == values.arrayHash, "clone lost the native array reflection hash");
    std::get<e::Value::Array>(copied.data).clear();
    check(std::get<e::Value::Array>(values.data).size() == 1, "clone shares array storage");
    // The native loader also needs enum identity in the array descriptor.
    // Parsed values alone cannot detect a lost class reference on export.
    e::Document enum_arrays;
    add(enum_arrays, "EnumHolder", "Choices", e::FieldType::enumeration,
        e::Value{e::Value::Array{e::Value{std::int64_t{-1}, {}}, e::Value{std::int64_t{2}, {}}}, 0x11223344});
    add(enum_arrays, "IntegerHolder", "Numbers", e::FieldType::int32,
        e::Value{e::Value::Array{e::Value{std::int64_t{3}, {}}}, 0x55667788});
    e::TypeDescriptor choice;
    choice.name = "Choice"; choice.nameHash = hash(choice.name);
    choice.guid.bytes.fill(std::byte{0x71}); choice.signature = hash(choice.name);
    choice.fieldIndex = static_cast<std::int32_t>(enum_arrays.fields.size());
    choice.encodedType = static_cast<std::uint16_t>(static_cast<unsigned>(e::FieldType::enumeration) << 4);
    choice.size = 4; choice.alignment = 4;
    enum_arrays.types.push_back(choice);
    (void)e::ensure_fixup_type(enum_arrays, 2);
    enum_arrays.fields[0].encodedType |= static_cast<std::uint16_t>(e::FieldCategory::array);
    enum_arrays.fields[0].classRef = 2;
    enum_arrays.fields[1].encodedType |= static_cast<std::uint16_t>(e::FieldCategory::array);
    // Reflection and fixup tables may number the same type differently.
    std::swap(enum_arrays.fixupTypeGuids[0], enum_arrays.fixupTypeGuids[2]);
    std::swap(enum_arrays.fixupTypeSignatures[0], enum_arrays.fixupTypeSignatures[2]);
    enum_arrays.instances[0].fixupType = 2;
    reflection(enum_arrays);
    const auto enum_round = e::read_document(e::write_document(enum_arrays, true));
    check(enum_round.arrays.size() == 2, "enum array export changed the record count");
    check(enum_round.arrays[0].classRef == 0 &&
          enum_round.fixupTypeGuids.at(enum_round.arrays[0].classRef) == choice.guid,
          "enum array export lost its native type identity");
    check(enum_round.arrays[1].classRef == 0xffff,
          "primitive array incorrectly acquired a type identity");
    const auto& choices = std::get<e::Value::Array>(
        enum_round.instances[0].object->find("Choices")->value.data);
    check(choices.size() == 2 && std::get<std::int64_t>(choices[0].data) == -1 &&
          std::get<std::int64_t>(choices[1].data) == 2, "enum array values changed");
    // Serialized type references index EFIX, whose GUID-only suffix moves
    // when an imported instance type adds a signature before it.
    e::Document references;
    add(references, "TypeHolder", "ReferencedType", e::FieldType::typeRef,
        e::Value{e::TypeReference{false, {}, 2, 10}, {}});
    add(references, "PrimitiveHolder", "PrimitiveType", e::FieldType::typeRef,
        e::Value{e::TypeReference{true, e::FieldType::float32, -1, 0x800001a0U}, {}});
    dingosdk::frostbite::Guid referenced_guid;
    referenced_guid.bytes.fill(std::byte{0x7a});
    references.fixupTypeGuids.push_back(referenced_guid);
    reflection(references);
    const auto source_bytes = e::write_document(references, true);
    references = e::read_document(source_bytes);
    auto imported_reference = e::with_imported_type(references, donor, "NativeLabel");
    e::detail::Carrier reference_carrier(donor, imported_reference, true);
    (void)reference_carrier.instance(1);
    const auto read_reference = e::read_document(e::write_document(imported_reference, true));
    const auto reference = std::get<e::TypeReference>(
        read_reference.instances[0].object->find("ReferencedType")->value.data);
    check(read_reference.fixupTypeGuids.at(reference.encoded >> 2) == referenced_guid,
          "import changed the identity of an existing GUID-only type reference");
    check(reference.encoded == 14 && reference.descriptor == 3,
          "type reference was not renumbered with its EFIX slot");
    const auto primitive = std::get<e::TypeReference>(
        read_reference.instances[1].object->find("PrimitiveType")->value.data);
    check(primitive.encoded == 0x800001a0U, "import changed a primitive type reference");
    check(e::write_document(references, true) == source_bytes,
          "fixup remapping mutated the original asset");
    // Opaque boxed payloads have their own relocation slots and are not
    // reached by the parsed field walk. Duplicate slots must move only once.
    auto opaque = e::with_imported_type(references, donor, "NativeLabel");
    opaque.payload.assign(16, std::byte{});
    const std::uint32_t old_type = 10, primitive_type = 0x800001a0U;
    std::memcpy(opaque.payload.data(), &old_type, 4);
    std::memcpy(opaque.payload.data() + 8, &primitive_type, 4);
    opaque.typeInfoOffsets = {0, 0, 8};
    opaque.boxedValues = {{0, 1, 0, 0, 2, opaque.payload}};
    opaque.arrays = {{0, 1, 0, 0, 2}, {0, 1, 0, 0, 0xffff}};
    const auto label_descriptor = static_cast<std::int32_t>(opaque.types.size() - 1);
    (void)e::ensure_fixup_type(opaque, label_descriptor);
    std::uint32_t payload_type{}, boxed_type{}, unchanged_primitive{};
    std::memcpy(&payload_type, opaque.payload.data(), 4);
    std::memcpy(&boxed_type, opaque.boxedValues[0].rawBytes.data(), 4);
    std::memcpy(&unchanged_primitive, opaque.payload.data() + 8, 4);
    check(payload_type == 14 && boxed_type == 14 && unchanged_primitive == primitive_type,
          "opaque type-reference relocation lost its identity or moved twice");
    check(opaque.arrays[0].classRef == 3 && opaque.arrays[1].classRef == 0xffff &&
          opaque.boxedValues[0].classRef == 3, "extra type records did not follow the moved suffix");
    std::cout << "Native type import: round trip, remapping, input preservation and refusals passed\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
