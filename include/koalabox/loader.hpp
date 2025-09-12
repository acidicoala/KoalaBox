#pragma once

#include <filesystem>
#include <map>

namespace koalabox::loader {
    namespace fs = std::filesystem;

    using export_map_t = std::map<std::string, std::string>;

    export_map_t get_export_map(void* library, bool undecorate = false);

    std::string get_decorated_function(void* library, const std::string& function_name);

    void* load_original_library(const fs::path& self_path, const std::string& orig_library_name);
}
