#pragma once

#include <filesystem>
#include <optional>
#include <set>
#include <string>

namespace koalabox::tools::module {
    using symbol_name_t = std::string;
    using exports_t = std::set<symbol_name_t>;
    std::optional<exports_t> get_exports(const std::filesystem::path& module_path);
    exports_t get_exports_or_throw(const std::filesystem::path& module_path);
}
