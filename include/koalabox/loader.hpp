#pragma once

#include <koalabox/koalabox.hpp>

namespace koalabox::loader {

    Path get_module_dir(const HMODULE& handle);

    Map<String, String> get_export_map(const HMODULE& library, bool undecorate = false);

    String get_decorated_function(const HMODULE& library, const String& function_name);

    HMODULE load_original_library(const Path& self_directory, const String& orig_library_name);

}
