#include "local_asset_pack.h"
#include "mod_merge_internal.h"
#include "game_bundles.h"
#include "Engine/Resource/ebx_writer.h"
#include "Engine/Resource/ebx_fixup_repair.h"
#include "Engine/Resource/cas_codec.h"
#include <stdexcept>

namespace dingosdk::mods {
namespace {
void write_changed(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    if (std::filesystem::is_regular_file(path) && std::filesystem::file_size(path) == bytes.size()) {
        const auto existing = detail::read_file(path);
        if (std::ranges::equal(existing, bytes)) return;
    }
    detail::write_file(path, bytes);
}
}
void make_local_ebx_pack(const std::filesystem::path& game_root, const std::filesystem::path& output,
                         std::string_view toc, std::string_view bundle,
                         std::span<const std::string_view> sources, const LocalEbxEdit& edit) {
    namespace fb = frostbite;
    if (sources.empty() || sources.size() > 8 || !edit) throw std::runtime_error("Invalid local asset recipe");
    const auto toc_path = std::filesystem::path(toc);
    if (toc_path.is_absolute() || std::ranges::any_of(toc_path, [](const auto& part) { return part == ".."; }))
        throw std::runtime_error("Local asset TOC is outside the pack");
    vfs::GameData game(game_root);
    const auto source = game.read_bundle(game.read_toc(toc), bundle);
    if (!source) throw std::runtime_error("Local asset source bundle missing");
    std::vector<fb::ebx::Document> documents;
    fb::BundleAsset asset;
    std::uint32_t install_chunk{};
    for (const auto name : sources) {
        std::size_t index{};
        const auto* entry = source->find(fb::AssetKind::ebx, name, &index);
        if (!entry || entry->originalSize > 2 * 1024 * 1024) throw std::runtime_error("Local asset source missing or too large");
        const auto* payload = source->payload(fb::AssetKind::ebx, index);
        if (!payload || payload->size > 2 * 1024 * 1024) throw std::runtime_error("Local asset source payload differs");
        if (documents.empty()) { asset = *entry; install_chunk = payload->location.installChunk; }
        documents.push_back(fb::ebx::read_document(game.read(*payload)));
    }
    const auto changed = edit(documents);
    if (changed.fileGuid != documents.front().fileGuid || !changed.root() || !documents.front().root() ||
        changed.root()->instanceGuid != documents.front().root()->instanceGuid)
        throw std::runtime_error("Local asset edit changed its identity");
    auto decoded = fb::ebx::write_document(changed, true);
    if (decoded.size() > 2 * 1024 * 1024) throw std::runtime_error("Local asset edit exceeds its size limit");
    // Same fixup validation as the mod merger before the native loader sees it.
    if (auto repair = fb::ebx::repair_type_fixups(decoded)) decoded = std::move(repair->bytes);
    const auto checked = fb::ebx::read_document(decoded);
    if (checked.fileGuid != changed.fileGuid || checked.instances.size() != changed.instances.size())
        throw std::runtime_error("Local asset failed its export check");
    const auto encoded = fb::encode_cas(decoded, {game_root, fb::CasCompression::raw});
    asset.originalSize = decoded.size(); asset.sha1 = detail::sha1_of(decoded);
    fb::BinaryBundle manifest; manifest.ebx.push_back(std::move(asset));
    const fb::BundleFileInfo payload{{true, install_chunk, 1}, 0, static_cast<std::uint32_t>(encoded.size())};
    fb::TocBundle replacement;
    replacement.name = bundle;
    replacement.region = fb::write_bundle_region(std::span(&payload, 1), fb::write_binary_bundle(manifest));
    const auto layout = vfs::read_layout(game_root / L"Data" / L"layout.toc");
    const vfs::GameArchives archives(game_root / L"Data", layout.root);
    const auto archive = detail::inside(output, std::filesystem::path("Win32") / archives.directory(install_chunk) / vfs::archive_file(1));
    write_changed(archive, encoded);
    write_changed(detail::inside(output, toc_path), fb::write_patch_toc(std::span(&replacement, 1)));
    // Publish the layout last. A partial factory never becomes a catalogue entry.
    write_changed(output / L"layout.toc", detail::read_file(game_root / L"Data" / L"layout.toc"));
}
}
