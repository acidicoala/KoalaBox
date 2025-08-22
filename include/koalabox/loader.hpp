#pragma once

#include <filesystem>
#include <map>

namespace koalabox::loader {

    namespace fs = std::filesystem;

    fs::path get_module_dir(const HMODULE& handle);

    std::map<std::string, std::string>
    get_export_map(const HMODULE& library, bool undecorate = false);

    std::string
    get_decorated_function(const HMODULE& library, const std::string& function_name);

    HMODULE
    load_original_library(const fs::path& self_path, const std::string& orig_library_name);

}
