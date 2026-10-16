// Offline, read-only metadata inspection of the shipped scoring managers.
#include "Extension/ArcadeMode/arcade_score_asset_inspect.h"
#include "Engine/Vfs/game_bundles.h"
#include <filesystem>
#include <iostream>
#include <set>

namespace {
namespace scoring = dingosdk::arcade_mode::score_assets;
namespace fb = dingosdk::frostbite;
using dingosdk::Json;
template<std::size_t N> std::string hex(const std::array<std::byte, N>& bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out; out.reserve(N * 2);
    for (auto byte : bytes) {
        const auto n = std::to_integer<unsigned>(byte); out += digits[n >> 4]; out += digits[n & 15];
    }
    return out;
}
Json identity(const fb::BundleAsset& asset, std::string_view bundle) {
    return Json{{"bundle", bundle}, {"name", asset.name}, {"sha1", hex(asset.sha1.bytes)},
                {"kind", asset.kind == fb::AssetKind::ebx ? "ebx" : "resource"},
                {"declared_bytes", asset.originalSize}, {"resource_id", asset.resourceId},
                {"resource_type", asset.resourceType}};
}
int inspect(const std::filesystem::path& root) {
    Json output{{"schema", 1}, {"kind", "arcade_scoring_asset_metadata"}, {"owner_proof", "UNVERIFIED"},
                {"graph_producer", "UNVERIFIED"}, {"graph_bytecode_decoded", false},
                {"scope", "fixed_scoring_manager_prefixes_in_two_core_bundles"},
                {"toc", scoring::toc_name}, {"targets", scoring::targets()}, {"assets", Json::array()},
                {"status", "NOT_FOUND"}};
    bool incomplete{}; std::size_t consumed{}, serialized{};
    try {
        constexpr std::size_t metadata_bytes = 64 * 1024 * 1024;
        for (const auto relative : {std::string_view{"layout.toc"}, scoring::toc_name})
            if (std::filesystem::file_size(root / "Data" / relative) > metadata_bytes)
                throw std::runtime_error("metadata_limit");
        const dingosdk::vfs::GameData data(root);
        const auto toc = data.read_toc(scoring::toc_name);
        if (toc.bundles.size() > 4096) throw std::runtime_error("bundle_limit");
        std::array<std::optional<dingosdk::vfs::GameBundle>, scoring::bundle_names.size()> bundles;
        scoring::AssetSelection selection;
        scoring::DeclarationInventory inventory;
        for (std::size_t b = 0; b < bundles.size(); ++b) {
            bundles[b] = data.read_bundle(toc, scoring::bundle_names[b]);
            const auto& bundle = bundles[b];
            if (!bundle) { incomplete = true; continue; }
            if (bundle->manifest.ebx.size() > 100000 || bundle->manifest.resources.size() > 100000) {
                incomplete = true; continue;
            }
            for (const auto kind : {fb::AssetKind::ebx, fb::AssetKind::resource}) {
                const auto& assets = kind == fb::AssetKind::ebx ? bundle->manifest.ebx : bundle->manifest.resources;
                for (std::size_t i = 0; i < assets.size(); ++i) {
                    selection.consider({assets[i].name, b, i, kind == fb::AssetKind::resource});
                    inventory.consider({assets[i].name, b, i, kind == fb::AssetKind::resource});
                }
            }
        }
        incomplete = incomplete || selection.omitted || inventory.omitted;
        output["asset_selection"] = "pinned_graphs_then_context_declarations_then_global_events_then_namespace";
        output["asset_limit_reached"] = selection.omitted;
        output["manifest_declarations"] = Json::array();
        output["manifest_name_limit_reached"] = inventory.omitted;
        for (std::size_t i = 0; i < inventory.size; ++i)
            output["manifest_declarations"].push_back(Json{{"name", inventory.assets[i].name},
                {"bundle", scoring::bundle_names[inventory.assets[i].bundle]}});
        output["declaration_lookups"] = Json::array();
        for (const auto& target : scoring::declaration_targets)
            output["declaration_lookups"].push_back(Json{{"role", target.role}, {"file_guid", target.file_guid},
                {"class_guid", target.class_guid}, {"status", "UNRESOLVED_IN_SELECTED_DECLARATIONS"}});
        serialized = output.dump().size();
        for (std::size_t selected = 0; selected < selection.size; ++selected) {
            const auto& candidate = selection.assets[selected];
            const auto& bundle = bundles[candidate.bundle];
            const auto bundle_name = scoring::bundle_names[candidate.bundle];
            const auto kind = candidate.resource ? fb::AssetKind::resource : fb::AssetKind::ebx;
            const auto& asset = candidate.resource ? bundle->manifest.resources[candidate.index] : bundle->manifest.ebx[candidate.index];
            const auto i = candidate.index;
            auto row = identity(asset, bundle_name);
            row["status"] = "RESOURCE_IDENTITY_ONLY";
            if (kind == fb::AssetKind::ebx) {
                const auto* payload = bundle->payload(kind, i);
                if (!payload || !asset.originalSize || asset.originalSize > scoring::payload_limit ||
                    payload->size > scoring::payload_limit ||
                    consumed > scoring::total_payload_limit - asset.originalSize) {
                    row["status"] = "PAYLOAD_UNAVAILABLE_OR_LIMIT"; incomplete = true;
                } else {
                    consumed += static_cast<std::size_t>(asset.originalSize);
                    try {
                        const auto bytes = data.read(*payload);
                        if (bytes.size() != asset.originalSize || bytes.size() > scoring::payload_limit)
                            throw std::runtime_error("payload_size");
                        const auto file_guid = fb::ebx::read_file_guid(bytes);
                        const auto document = fb::ebx::read_document(bytes);
                        if (document.fileGuid != file_guid) throw std::runtime_error("file_guid_mismatch");
                        row["declarations"] = scoring::inspect_document(document);
                        for (std::size_t target = 0; target < scoring::declaration_targets.size(); ++target) {
                            if (!scoring::matches_declaration(document, scoring::declaration_targets[target])) continue;
                            auto& lookup = output["declaration_lookups"][target];
                            lookup["status"] = "DECLARATION_IDENTITY_RESOLVED";
                            lookup["name"] = asset.name; lookup["bundle"] = bundle_name;
                        }
                        row["status"] = row["declarations"]["status"];
                        if (row["status"].string() != "DECLARATIONS_READ") incomplete = true;
                        // References identify RES candidates without reading or decoding their payloads.
                        std::set<std::uint64_t> referenced;
                        for (const auto& ref : row["declarations"]["references"])
                            if (ref["kind"].string() == "resource") referenced.insert(ref["resource_id"].get<std::uint64_t>());
                        row["referenced_resources"] = Json::array();
                        for (const auto& resource : bundle->manifest.resources) {
                            if (!referenced.contains(resource.resourceId)) continue;
                            if (!scoring::allowed_asset(resource.name)) {
                                row["external_resource_reference"] = true; incomplete = true; continue;
                            }
                            if (row["referenced_resources"].size() >= 64) { incomplete = true; break; }
                            row["referenced_resources"].push_back(identity(resource, bundle_name));
                            referenced.erase(resource.resourceId);
                        }
                        if (!referenced.empty()) { row["unresolved_resource_references"] = referenced.size(); incomplete = true; }
                    } catch (const std::exception&) {
                        // Exception messages may contain paths or opaque payload text.
                        row = identity(asset, bundle_name); row["status"] = "READ_OR_PARSE_FAILED"; incomplete = true;
                    }
                }
            }
            const auto size = row.dump().size();
            if (size > scoring::output_limit / 2 || serialized + size > scoring::output_limit - 8192) {
                row = identity(asset, bundle_name); row["status"] = "OUTPUT_LIMIT"; incomplete = true;
            }
            serialized += row.dump().size(); output["assets"].push_back(std::move(row));
        }
        for (const auto& lookup : output["declaration_lookups"])
            if (lookup["status"].string() != "DECLARATION_IDENTITY_RESOLVED") incomplete = true;
        output["status"] = incomplete ? "INCOMPLETE" : output["assets"].empty() ? "NOT_FOUND" : "METADATA_ONLY";
    } catch (const std::exception&) { output["status"] = "INCOMPLETE"; output["reason"] = "FIXED_BUNDLE_READ_FAILED"; }
    output["declared_payload_bytes_read"] = consumed;
    const auto text = output.dump();
    if (text.size() > scoring::output_limit) {
        std::cout << "{\"schema\":1,\"status\":\"INCOMPLETE\",\"reason\":\"OUTPUT_LIMIT\",\"owner_proof\":\"UNVERIFIED\"}\n";
        return 1;
    }
    std::cout << text << '\n';
    return output["status"].string() == "METADATA_ONLY" ? 0 : 1;
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: dingosdk_arcade_score_assets <game-root>\n"; return 2; }
    return inspect(std::filesystem::path(argv[1]));
}
