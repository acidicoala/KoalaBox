#pragma once

#include <koalabox/core.hpp>

namespace koalabox::hook {

    KOALABOX_API(void) detour_or_throw(
        Map<String, uintptr_t>& address_map,
        uintptr_t address,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour_or_throw(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour_or_warn(
        Map<String, uintptr_t>& address_map,
        uintptr_t address,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour_or_warn(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour(
        Map<String, uintptr_t>& address_map,
        uintptr_t address,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) eat_hook_or_throw(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) eat_hook_or_warn(
        Map<String, uintptr_t>& address_map,
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) swap_virtual_func_or_throw(
        Map<String, uintptr_t>& address_map,
        const void* instance,
        const String& function_name,
        int ordinal,
        uintptr_t callback_function
    );

    KOALABOX_API(void) swap_virtual_func(
        Map<String, uintptr_t>& address_map,
        const void* instance,
        const String& function_name,
        int ordinal,
        uintptr_t callback_function
    );

    uintptr_t get_original_function(
        const HMODULE& library,
        const char* function_name
    );

    uintptr_t get_original_hooked_function(
        const Map<String, uintptr_t>& address_map,
        const char* function_name
    );

    template<typename F>
    F get_original_function(const HMODULE& library, const char* function_name, F) {
        return reinterpret_cast<F>(get_original_function(library, function_name));
    }

    template<typename F>
    F get_original_hooked_function(const Map<String, uintptr_t>& address_map, const char* function_name, F) {
        return reinterpret_cast<F>(get_original_hooked_function(address_map, function_name));
    }

    KOALABOX_API(void) init(bool print_info = false);

    KOALABOX_API(bool) is_hook_mode(const HMODULE& self_module, const String& orig_library_name);

}
