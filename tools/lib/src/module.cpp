#include <koalabox/core.hpp>
#include <koalabox/path.hpp>

#include <koalabox_tools/module.hpp>

namespace koalabox::tools::module {
    exports_t get_exports_or_throw(const std::filesystem::path& module_path) {
        return get_exports(module_path) | throw_if_empty(
                   std::format("Failed get library exports of {}", path::to_str(module_path))
               );
    }
}
