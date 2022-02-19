#pragma once

#include "koalabox/koalabox.hpp"
#include "koalabox/hook/hook.hpp"

namespace koalabox::loader {

    Path get_module_dir(const HMODULE& handle);

    Map<String, String> get_undecorated_function_map(const HMODULE& library);

    String get_undecorated_function(const HMODULE& library, const String& function_name);

    HMODULE load_original_library(const Path& self_directory, const String& orig_library_name);

}
