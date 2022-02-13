#pragma once

#include "koalabox/koalabox.hpp"

namespace koalabox::loader {

    using namespace koalabox;

    [[maybe_unused]]
    Path get_module_dir(HMODULE& handle);

    [[maybe_unused]]
    HMODULE load_original_library(const Path& self_directory, const String& orig_library_name);

}
