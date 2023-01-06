#pragma once

#include <koalabox/util.hpp>

namespace koalabox::hook {

    void detour_or_throw(
        Map<String, FunctionAddress>& address_map,
        FunctionAddress address,
        const String& function_name,
        FunctionAddress callback_function
    );

    void detour_or_throw(
        const HMODULE& module_handle,
        const String& function_name,
        FunctionAddress callback_function
    );

    void detour_or_warn(
        Map<String, FunctionAddress>& address_map,
        FunctionAddress address,
        const String& function_name,
        FunctionAddress callback_function
    );

    void detour_or_warn(
        const HMODULE& module_handle,
        const String& function_name,
        FunctionAddress callback_function
    );

    void detour(
        Map<String, FunctionAddress>& address_map,
        FunctionAddress address,
        const String& function_name,
        FunctionAddress callback_function
    );

    void detour(
        const HMODULE& module_handle,
        const String& function_name,
        FunctionAddress callback_function
    );

    void eat_hook_or_throw(
        Map<String, FunctionAddress>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        FunctionAddress callback_function
    );

    void eat_hook_or_warn(
        Map<String, FunctionAddress>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        FunctionAddress callback_function
    );

    void swap_virtual_func_or_throw(
        Map<String, FunctionAddress>& address_map,
        const void* instance,
        const String& function_name,
        int ordinal,
        FunctionAddress callback_function
    );

    void swap_virtual_func(
        Map<String, FunctionAddress>& address_map,
        const void* instance,
        const String& function_name,
        int ordinal,
        FunctionAddress callback_function
    );

    FunctionAddress get_original_function(const HMODULE& library, const char* function_name);
    FunctionAddress get_original_function(const Map<String, FunctionAddress>& address_map, const char* function_name);

    template<typename F>
    F get_original_function(const HMODULE& library, const char* function_name, F) {
        return reinterpret_cast<F>(get_original_function(library, function_name));
    }

    template<typename F>
    F get_original_function(const Map<String, FunctionAddress>& address_map, const char* function_name, F) {
        return reinterpret_cast<F>(get_original_function(address_map, function_name));
    }

    void init(bool print_info = false);

    bool is_hook_mode(const HMODULE& self_module, const String& orig_library_name);

}
