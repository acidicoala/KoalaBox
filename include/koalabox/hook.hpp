#pragma once

#include <string>

namespace koalabox::hook {

    void detour_or_throw(
        uintptr_t address, const std::string& function_name, uintptr_t callback_function
    );

    void detour_or_throw(
        const HMODULE& module_handle, const std::string& function_name, uintptr_t callback_function
    );

    void detour_or_warn(
        uintptr_t address, const std::string& function_name, uintptr_t callback_function
    );

    void detour_or_warn(
        const HMODULE& module_handle, const std::string& function_name, uintptr_t callback_function
    );

    void detour(uintptr_t address, const std::string& function_name, uintptr_t callback_function);

    void detour(
        const HMODULE& module_handle, const std::string& function_name, uintptr_t callback_function
    );

    void eat_hook_or_throw(
        const HMODULE& module_handle, const std::string& function_name, uintptr_t callback_function
    );

    void eat_hook_or_warn(
        const HMODULE& module_handle, const std::string& function_name, uintptr_t callback_function
    );

    void swap_virtual_func_or_throw(
        const void* instance, const std::string& function_name, int ordinal,
        uintptr_t callback_function
    );

    void swap_virtual_func(
        const void* instance, const std::string& function_name, int ordinal,
        uintptr_t callback_function
    );

    uintptr_t get_original_function(const HMODULE& library, const std::string& function_name);

    uintptr_t get_original_address(const std::string& function_name);

    template <typename F>
    F get_original_function(const HMODULE& library, const std::string& function_name, F) {
        return reinterpret_cast<F>(get_original_function(library, function_name));
    }

    template <typename F> F get_original_hooked_function(const std::string& function_name, F) {
        return reinterpret_cast<F>(get_original_address(function_name));
    }

    void init(bool print_info = false);

    bool is_hook_mode(const HMODULE& self_module, const std::string& orig_library_name);

}
