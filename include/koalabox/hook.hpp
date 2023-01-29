#pragma once

#include <koalabox/core.hpp>

namespace koalabox::hook {

    KOALABOX_API(void) detour_or_throw(
        uintptr_t address,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour_or_throw(
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour_or_warn(
        uintptr_t address,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour_or_warn(
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour(
        uintptr_t address,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) detour(
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) eat_hook_or_throw(
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) eat_hook_or_warn(
        const HMODULE& module_handle,
        const String& function_name,
        uintptr_t callback_function
    );

    KOALABOX_API(void) swap_virtual_func_or_throw(
        const void* instance,
        const String& function_name,
        int ordinal,
        uintptr_t callback_function
    );

    KOALABOX_API(void) swap_virtual_func(
        const void* instance,
        const String& function_name,
        int ordinal,
        uintptr_t callback_function
    );

    KOALABOX_API(uintptr_t) get_original_function(
        const HMODULE& library,
        const String& function_name
    );

    KOALABOX_API(uintptr_t) get_original_hooked_function(const String& function_name);

    template<typename F>
    KOALABOX_API(F) get_original_function(const HMODULE& library, const String& function_name, F) {
        return reinterpret_cast<F>(get_original_function(library, function_name));
    }

    template<typename F>
    KOALABOX_API(F)
    get_original_hooked_function(const String& function_name, F) {
        return reinterpret_cast<F>(get_original_hooked_function(function_name));
    }

    KOALABOX_API(void) init(bool print_info = false);

    KOALABOX_API(bool) is_hook_mode(const HMODULE& self_module, const String& orig_library_name);

}
