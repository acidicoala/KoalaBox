#pragma once

#include "koalabox/koalabox.hpp"
#include "koalabox/hook/hook.hpp"

namespace koalabox::loader {

    using namespace koalabox;

    [[maybe_unused]]
    Path get_module_dir(HMODULE& handle);

    [[maybe_unused]]
    hook::FunctionPointer get_original_function(
        bool is_hook_mode,
        HMODULE library,
        const String& function_name
    );

    template<typename F>
    [[maybe_unused]]
    F get_original_function(
        bool is_hook_mode,
        HMODULE library,
        const String& function_name,
        F
    ) {
        return reinterpret_cast<F>(get_original_function(is_hook_mode, library, function_name));
    }

    [[maybe_unused]]
    String get_undecorated_function(HMODULE library, const String& function_name);

    [[maybe_unused]]
    HMODULE load_original_library(const Path& self_directory, const String& orig_library_name);

}
