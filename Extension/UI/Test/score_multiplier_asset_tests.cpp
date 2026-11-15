// Exercise the production listener clone with entirely authored graph records.
#include "../score_multiplier_asset.cpp"
#include <iostream>

namespace dingosdk::score_line {
e::Value connection(std::size_t from, std::size_t to, std::int64_t output, std::int64_t input) {
    auto value = std::make_shared<e::Object>();
    value->fields = {{0, "Source", reference(from)}, {1, "Target", reference(to)},
        {2, "SourceFieldId", {output, {}}}, {3, "TargetFieldId", {input, {}}},
        {4, "Flags", {std::uint64_t{2}, {}}}};
    return {value, {}};
}
void test_listener_inputs() {
    e::Document d;
    e::InstanceRecord root, listener;
    root.object = std::make_shared<e::Object>();
    listener.object = std::make_shared<e::Object>();
    listener.object->fields = {{0, "Flags", {std::uint64_t{0}, {}}}};
    root.object->fields = {{0, "PropertyConnections", {e::Value::Array{
        connection(2, 1, 11, 21), connection(2, 1, 12, 22),
        connection(1, 2, 31, 41)}, {}}}};
    d.instances = {root, listener, listener};
    const auto copy = clone_listener(d, 1, 2);
    const auto& edges = array(object(d, 0), "PropertyConnections");
    require(copy == 3 && edges.size() == 5, "Listener did not retain both incoming bindings");
    for (std::size_t i = 0; i < 2; ++i) {
        const auto& original = *std::get<std::shared_ptr<e::Object>>(edges[i].data);
        const auto& added = *std::get<std::shared_ptr<e::Object>>(edges[i + 3].data);
        require(referred(field(original, "Target")) == 1, "Changed original context binding");
        require(referred(field(added, "Target")) == copy, "Cloned binding targets the wrong listener");
        require(referred(field(added, "Source")) == 2, "Changed the context owner");
        require(integer(field(added, "SourceFieldId")) == 11 + i &&
                integer(field(added, "TargetFieldId")) == 21 + i &&
                integer(field(added, "Flags")) == 2, "Changed a binding port or flags");
    }
    field(*std::get<std::shared_ptr<e::Object>>(array(object(d, 0), "PropertyConnections")[3].data),
          "SourceFieldId").data = std::int64_t{99};
    require(integer(field(*std::get<std::shared_ptr<e::Object>>(edges[0].data), "SourceFieldId")) == 11,
            "Cloned edge aliases the original");
    const auto before = d.instances.size();
    array(object(d, 0), "PropertyConnections").clear();
    bool refused = false;
    try { (void)clone_listener(d, 1, 3); } catch (const std::runtime_error&) { refused = true; }
    require(refused && d.instances.size() == before, "Created an unbound listener");
}
}
int main() try {
    dingosdk::score_line::test_listener_inputs();
    std::cout << "Listener context, enable input, independent edges and missing-input refusal passed\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
