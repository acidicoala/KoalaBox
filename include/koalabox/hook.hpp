#pragma once

#include <koalabox/util.hpp>

namespace koalabox::hook {

    extern Map<String, FunctionAddress> address_book;

    void detour_or_throw(const HMODULE& module, const String& function_name, FunctionAddress callback_function);

    void detour_or_warn(const HMODULE& module, const String& function_name, FunctionAddress callback_function);

    void detour(const HMODULE& module, const String& function_name, FunctionAddress callback_function);

    void eat_hook_or_throw(const HMODULE& module, const String& function_name, FunctionAddress callback_function);

    void eat_hook_or_warn(const HMODULE& module, const String& function_name, FunctionAddress callback_function);

    void swap_virtual_func_or_throw(
        const void* instance,
        const String& function_name,
        int ordinal,
        FunctionAddress callback_function
    );

    void swap_virtual_func(
        const void* instance,
        const String& function_name,
        int ordinal,
        FunctionAddress callback_function
    );

    FunctionAddress get_original_function(bool is_hook_mode, const HMODULE& library, const String& function_name);

    template<typename F>
    F get_original_function(bool is_hook_mode, const HMODULE& library, const String& function_name, F) {
        return reinterpret_cast<F>(get_original_function(is_hook_mode, library, function_name));
    }

    void init(bool print_info = false);

    bool is_hook_mode(const HMODULE& self_module, const String& orig_library_name);

}
