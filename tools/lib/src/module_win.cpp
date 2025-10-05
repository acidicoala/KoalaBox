// Need to define this to avoid conflict errors
#define _PEPARSE_WINDOWS_CONFLICTS // NOLINT(*-reserved-identifier, *-dcl37-c, *-dcl51-cpp)
#include <pe-parse/parse.h>

#include <koalabox/path.hpp>

#include "koalabox_tools/module.hpp"

namespace koalabox::tools::module {
    std::optional<exports_t> get_exports(const std::filesystem::path& module_path) {
        const auto lib_path_str = path::to_str(module_path);

        auto* pe = peparse::ParsePEFromFile(lib_path_str.c_str());

        const auto callback = [](
            void* context,
            const peparse::VA& /*funcAddr*/,
            std::uint16_t /*ordinal*/,
            const std::string& /*mod*/,
            const std::string& func,
            const std::string& /*fwd*/ // @formatter:off
        ) {// @formatter:on
            auto* results_ptr = static_cast<exports_t*>(context);
            results_ptr->insert(func);
            return 0;
        };

        exports_t results;
        peparse::IterExpFull(pe, callback, &results);
        peparse::DestructParsedPE(pe);

        return results;
    }
}
