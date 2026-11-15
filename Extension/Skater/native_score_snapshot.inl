// Included beside the existing HUD reader, under the native registry lock.
namespace native_scoring_read {
namespace layout = addr::native_scoring;
using menu_data::Context;
using menu_data::Value;
using menu_data::require;
using menu_data::read;
void schema(Address type, std::uint32_t hash, unsigned bytes) {
    require(type && menu_data::kind(type) == 2 && menu_data::size(type) == bytes &&
        read<std::uint32_t>(read<Address>(type)) == hash, "Native scoring schema differs.");
}
Value field(const Context& c, Value parent, std::uint32_t hash, unsigned kind, unsigned bytes) {
    const auto value = c.field(parent, hash);
    require(menu_data::kind(value.type) == kind && menu_data::size(value.type) == bytes,
        "Native scoring field differs.");
    return value;
}
Address array_type(Value array, unsigned kind, unsigned bytes, std::uint32_t hash) {
    require(menu_data::kind(array.type) == 4, "Native scoring array differs.");
    const auto element = read<Address>(read<Address>(array.type) + layout::array_element_type);
    require(element && menu_data::kind(element) == kind &&
        (kind == 3 ? sizeof(Address) : menu_data::size(element)) == bytes &&
        read<std::uint32_t>(read<Address>(element)) == hash, "Native scoring array element differs.");
    return element;
}
NativeScoreSequence sequence(const Context& c, Value value) {
    schema(value.type, layout::sequence_schema, layout::sequence_size);
    NativeScoreSequence result;
    result.points = read<std::uint32_t>(c.address(field(c, value, layout::sequence_points, 16, 4)));
    result.index = read<std::uint32_t>(c.address(field(c, value, layout::sequence_index, 16, 4)));
    result.kind = read<std::uint32_t>(c.address(field(c, value, layout::sequence_kind, 8, 4)));
    result.current_action = read<Address>(c.address(field(c, value, layout::current_action, 3, sizeof(Address))));
    const auto actions = field(c, value, layout::sequence_actions, 4, sizeof(Address));
    (void)array_type(actions, 3, sizeof(Address), layout::action_schema);
    unsigned count{}, stride{};
    const auto bytes = c.array(actions, 64, count, stride);
    require(stride == sizeof(Address) && bytes.size() == std::size_t{count} * stride,
        "Native scoring action storage differs.");
    result.action_objects.resize(count);
    if (count) std::memcpy(result.action_objects.data(), bytes.data(), bytes.size());
    require(std::all_of(result.action_objects.begin(), result.action_objects.end(),
        [](auto action) { return action != 0; }), "Native scoring action is unavailable.");
    return result;
}
std::vector<NativeScoreSequence> line(const Context& c, Value value) {
    schema(value.type, layout::line_schema, layout::line_size);
    const auto sequences = field(c, value, layout::line_sequences, 4, sizeof(Address));
    (void)array_type(sequences, 2, layout::sequence_size, layout::sequence_schema);
    unsigned count{}, stride{};
    const auto bytes = c.array(sequences, 64, count, stride);
    require(stride == layout::sequence_size && bytes.size() == std::size_t{count} * stride,
        "Native scoring sequence storage differs.");
    std::vector<NativeScoreSequence> result;
    result.reserve(count);
    for (unsigned i = 0; i < count; ++i) result.push_back(sequence(c, c.element(sequences, i)));
    return result;
}
std::optional<NativeScoreSnapshot> snapshot(const Context& c, Value hud) {
    schema(hud.type, addr::hud_corner::score_view_schema, addr::score_hud::score_view_size);
    const auto reference = c.field(hud, addr::score_hud::scoring_data);
    require(menu_data::size(reference.type) == sizeof(menu_data::Ref), "Native scoring reference differs.");
    const auto before = read<menu_data::Ref>(c.address(reference));
    if (!before.handle) return std::nullopt;
    require(before.record != 0, "Native scoring reference is incomplete.");
    const Value scoring{before.handle, c.type_of(before.handle)};
    schema(scoring.type, layout::schema, layout::size);
    NativeScoreSnapshot result;
    result.hud = hud.handle; result.scoring = scoring.handle; result.manager = c.manager;
    result.current = sequence(c, c.field(scoring, layout::current_sequence));
    result.previous = sequence(c, c.field(scoring, layout::previous_sequence));
    result.completed = line(c, c.field(scoring, layout::current_line));
    result.previous_line = line(c, c.field(scoring, layout::previous_line));
    const auto after = read<menu_data::Ref>(c.address(reference));
    require(before.record == after.record && before.handle == after.handle &&
        c.type_of(scoring.handle) == scoring.type && c.type_of(hud.handle) == hud.type,
        "Native scoring owner changed.");
    return result;
}
}
