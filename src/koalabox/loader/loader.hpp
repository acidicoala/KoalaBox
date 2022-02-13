#pragma once

#include "koalabox/koalabox.hpp"

namespace loader {

    using namespace koalabox;

    [[maybe_unused]]
    Path get_module_dir(HMODULE& handle);

    [[maybe_unused]]
    bool is_hook_mode(HMODULE self_module, const String& orig_library_name);

    [[maybe_unused]]
    HMODULE load_original_library(const Path& self_directory, const String& orig_library_name);

}
