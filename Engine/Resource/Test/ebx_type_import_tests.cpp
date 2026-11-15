#include "Engine/Resource/ebx_type_import.h"
#include "Engine/Resource/ebx_carry.h"
#include "Engine/Resource/ebx_writer.h"
#include <iostream>
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
    std::cout << "Native type import: round trip, remapping, input preservation and refusals passed\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
