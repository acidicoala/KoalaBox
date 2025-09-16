#include "koalabox/loader.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/module.hpp"
#include "koalabox/path.hpp"

namespace koalabox::loader {
    void* load_original_library(const fs::path& self_path, const std::string& orig_library_name) {
#ifdef KB_WIN
        constexpr auto extension = "_o.dll";
#elifdef KB_LINUX
        constexpr auto extension = "_o.so";
#endif
        const auto full_original_library_name = orig_library_name + extension;
        const auto original_module_path = self_path / full_original_library_name;

        auto* const original_module = module::load_library_or_throw(original_module_path);

        LOG_INFO("Loaded original library: '{}'", full_original_library_name);
        LOG_TRACE("Loaded original library from: '{}'", path::to_str(original_module_path));

        return original_module;
    }
}
