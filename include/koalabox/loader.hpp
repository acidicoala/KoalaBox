#pragma once

#include <koalabox/core.hpp>

namespace koalabox::loader {

    KOALABOX_API(Path) get_module_dir(const HMODULE& handle);

    KOALABOX_API(Map<String, String>) get_export_map(const HMODULE& library, bool undecorate = false);

    KOALABOX_API(String) get_decorated_function(const HMODULE& library, const String& function_name);

    KOALABOX_API(HMODULE) load_original_library(const Path& self_path, const String& orig_library_name);

}
