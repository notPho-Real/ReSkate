#pragma once
#include "Engine/Resource/ebx_document.h"
#include <filesystem>
#include <functional>
#include <span>
#include <string_view>
namespace dingosdk::mods {
using LocalEbxEdit = std::function<frostbite::ebx::Document(std::span<const frostbite::ebx::Document>)>;
// The first named asset is amended; the rest are read-only donors from the same
// installed bundle. Writes only the private pack, never the source installation.
// Equal files are retained so the ordinary mod-merge cache remains effective.
void make_local_ebx_pack(const std::filesystem::path& game_root, const std::filesystem::path& output,
                         std::string_view toc, std::string_view bundle,
                         std::span<const std::string_view> sources, const LocalEbxEdit& edit);
}
