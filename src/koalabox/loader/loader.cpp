#include "loader.hpp"
#include "../win_util/win_util.hpp"
#include "../util/util.hpp"
#include "../logger/logger.hpp"

namespace loader {

    [[maybe_unused]]
    Path get_module_dir(HMODULE& handle) {
        auto file_name = win_util::get_module_file_name(handle);

        auto module_path = Path(file_name);

        return module_path.parent_path();
    }

    [[maybe_unused]]
    bool is_hook_mode(
        HMODULE self_module,
        const String& orig_library_name
    ) {
        const auto module_path = win_util::get_module_file_name(self_module);

        const auto self_name = Path(module_path).filename().string();

        return not util::strings_are_equal(self_name, orig_library_name + ".dll");
    }

    [[maybe_unused]]
    HMODULE load_original_library(
        const Path& self_directory,
        const String& orig_library_name
    ) {
        const auto original_module_path = self_directory / (orig_library_name + "_o.dll");

        const auto original_module = win_util::load_library(original_module_path);

        logger::info("📚 Loaded original library from: '{}'", original_module_path.string());

        return original_module;
    }

}
