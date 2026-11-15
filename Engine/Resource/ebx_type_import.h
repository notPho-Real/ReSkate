#pragma once
#include "ebx_document.h"
#include <string_view>

namespace dingosdk::frostbite::ebx {
// Add a donor's non-array type and its dependencies to a private document copy.
// Existing type indices and array reflection mappings stay unchanged. This is
// deliberately narrower than a schema merger: conflicting types and new array
// layouts are refused. The caller can then use Carrier for the actual instance.
[[nodiscard]] Document with_imported_type(const Document& target, const Document& donor,
                                          std::string_view type_name);
}
