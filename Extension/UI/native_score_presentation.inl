// Included by the existing model-write hook; all registry access remains under
// its native lock. The extra lock serializes replacement of different registries.
namespace score_presentation {
namespace score = addr::score_hud;
using menu_data::Context;
using menu_data::require;
using menu_data::read;
struct Parent { menu_data::Value value; unsigned offset{}; };
struct Field {
    menu_data::Value value;
    std::vector<Parent> parents;
    unsigned kind{}, bytes{};
    Value original{}, shown{};
    std::string original_text, shown_text;
    bool original_null{}, held{};
};
struct State {
    std::recursive_mutex mutex;
    Address manager{}, base{};
    menu_data::Value root;
    std::array<Field, 6> fields;
};
State& state() { static auto* value = new State; return *value; }
struct Publishing {
    bool previous = std::exchange(publishing, true);
    ui::OwnUiWrites writes;
    ~Publishing() { publishing = previous; }
};
// Engine strings are retained by the synchronous native typed write. Never
// pass std::string::c_str() as if it were an engine-owned CString.
struct Text {
    Address base{}, value{};
    Text(Address image, const std::string& text) : base(image) {
        require(text.size() <= 2048, "Score presentation text exceeds its limit.");
        game::native_data().values.assign(&value, text.c_str(), static_cast<std::uint32_t>(text.size()));
        require(value != 0, "Score presentation text is unavailable.");
    }
    ~Text() {
        if (value) reinterpret_cast<void (*)(Address*)>(base + addr::engine::native_text_release)(&value);
    }
    Text(const Text&) = delete;
    Text& operator=(const Text&) = delete;
};
bool write(Address manager, std::uint64_t handle, Address type, const void* value,
           std::uint8_t flag, ModelWrite original, std::uint64_t& result) {
    auto& s = state(); std::lock_guard lock(s.mutex);
    if (manager != s.manager) return false;
    std::vector<std::byte> bytes;
    std::vector<std::unique_ptr<Text>> strings;
    for (auto& field : s.fields) {
        if (!field.held) continue;
        const auto parent = std::find_if(field.parents.begin(), field.parents.end(), [&](const Parent& p) {
            return p.value.handle == handle && p.value.type == type;
        });
        const bool direct = field.value.handle == handle && field.value.type == type;
        if (!direct && parent == field.parents.end()) continue;
        const auto offset = direct ? 0u : parent->offset;
        if (bytes.empty()) {
            const auto size = menu_data::size(type);
            require(size && size <= 4096, "Score presentation write differs.");
            bytes.resize(size);
            require(memory::peek_bytes(reinterpret_cast<Address>(value), bytes.data(), bytes.size()),
                "Score presentation write is unavailable.");
        }
        require(offset <= bytes.size() && field.bytes <= bytes.size() - offset,
            "Score presentation parent differs.");
        const auto* incoming = static_cast<const std::byte*>(value) + offset;
        if (field.kind == 7) {
            Address text{}; std::memcpy(&text, incoming, sizeof text);
            field.original = text;
            field.original_text = text ? menu_data::string(text, 2048) : std::string{};
            field.original_null = !text;
            strings.push_back(std::make_unique<Text>(s.base, field.shown_text));
            std::memcpy(bytes.data() + offset, &strings.back()->value, sizeof(Address));
        } else {
            std::memcpy(&field.original, incoming, field.bytes);
            std::memcpy(bytes.data() + offset, &field.shown, field.bytes);
        }
    }
    if (bytes.empty()) return false;
    Publishing ours;
    result = original(manager, handle, type, bytes.data(), flag);
    return true;
}
void clear(State& s) {
    s.root = {}; s.fields = {}; s.manager = s.base = 0;
}
void release(State& s, const Context& c) {
    if (s.manager != c.manager || !s.root.handle || c.type_of(s.root.handle) != s.root.type) { clear(s); return; }
    Publishing ours;
    for (auto& field : s.fields) {
        if (!field.held) continue;
        if (c.type_of(field.value.handle) != field.value.type) { field.held = false; continue; }
        if (field.kind == 7 && !field.original_null) c.text(field.value, field.original_text);
        else c.publish(field.value, &field.original);
        field.held = false;
    }
    clear(s);
}
Field bind(const Context& c, menu_data::Value root, std::initializer_list<std::uint32_t> path,
           unsigned kind, unsigned bytes) {
    Field result;
    auto value = root;
    for (const auto key : path) {
        const auto member = c.member(value.type, key);
        for (auto& parent : result.parents) parent.offset += member.offset;
        result.parents.push_back({value, member.offset});
        value = c.field(value, key);
    }
    require(menu_data::kind(value.type) == kind && menu_data::size(value.type) == bytes,
        "Score presentation field differs.");
    result.value = value; result.kind = kind; result.bytes = bytes;
    require(memory::peek_bytes(c.address(value), &result.original, bytes), "Score presentation field is unavailable.");
    if (kind == 7) {
        result.original_null = !result.original;
        if (!result.original_null) result.original_text = c.text(value, 2048);
    }
    return result;
}
void apply(const Context& c, menu_data::Value root, const std::optional<ScorePresentation>& wanted) {
    auto& s = state(); std::lock_guard lock(s.mutex);
    if (s.manager && s.manager != c.manager) clear(s);
    if (!wanted || wanted->manager != c.manager || wanted->hud != root.handle || !wanted->scoring ||
        !root.handle || c.type_of(root.handle) != root.type) { release(s, c); return; }
    const auto reference = c.field(root, score::scoring_data);
    require(menu_data::size(reference.type) == sizeof(menu_data::Ref), "Score presentation owner differs.");
    const auto owner = read<menu_data::Ref>(c.address(reference));
    if (!owner.record || owner.handle != wanted->scoring) { release(s, c); return; }
    if (s.root.handle != root.handle || s.root.type != root.type) {
        release(s, c);
        std::array<Field, 6> fields{{
            bind(c, root, {score::multiplier}, 19, 4),
            bind(c, root, {score::line_score}, 19, 4),
            bind(c, root, {score::action_points, score::label_text}, 7, sizeof(Address)),
            bind(c, root, {score::multiplier_text, score::label_text}, 7, sizeof(Address)),
            bind(c, root, {score::ring_on}, 10, 1),
            bind(c, root, {score::action_fill}, 19, 4),
        }};
        s.fields = std::move(fields); s.manager = c.manager; s.base = c.base; s.root = root;
    }
    const auto bits = [](float value) { Value result{}; std::memcpy(&result, &value, sizeof value); return result; };
    s.fields[0].shown = bits(static_cast<float>(wanted->multiplier));
    s.fields[1].shown = bits(static_cast<float>(std::min<double>(static_cast<double>(wanted->total), score_line::most_points)));
    s.fields[2].shown_text = std::to_string(wanted->points);
    s.fields[3].shown_text = wanted->multiplier ? std::to_string(wanted->multiplier) : std::string{};
    s.fields[4].shown = wanted->multiplier ? 1u : 0u;
    s.fields[5].shown = bits(static_cast<float>(std::min(1.0, static_cast<double>(wanted->points) / score::action_fill_points)));
    Publishing ours;
    for (auto& field : s.fields) {
        field.held = true; // publication must be reversible even if a later field fails
        if (field.kind == 7) {
            if (c.text(field.value, 2048) != field.shown_text) c.text(field.value, field.shown_text);
        } else {
            Value current{};
            require(memory::peek_bytes(c.address(field.value), &current, field.bytes), "Score presentation field expired.");
            if (current != field.shown) c.publish(field.value, &field.shown);
        }
    }
}
}
