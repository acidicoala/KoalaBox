#pragma once

#include <string>

#define KB_HOOK_GET_MODULE_FN(MODULE, FUNC) koalabox::hook::get_module_function(MODULE, #FUNC, FUNC)
#define KB_HOOK_GET_HOOKED_FN(FUNC) koalabox::hook::get_hooked_function(#FUNC, FUNC)

#define KB_HOOK_DETOUR_ADDRESS(FUNC, ADDRESS) \
    koalabox::hook::detour_or_warn(ADDRESS, #FUNC, reinterpret_cast<uintptr_t>(FUNC))

#define KB_HOOK_DETOUR_MODULE(FUNC, MODULE) \
    koalabox::hook::detour_or_warn(MODULE, #FUNC, reinterpret_cast<uintptr_t>(FUNC))

namespace koalabox::hook {
    bool is_hooked(const std::string& function_name);

    bool unhook(const std::string& function_name);

    void detour_or_throw(
        uintptr_t address,
        const std::string& function_name,
        uintptr_t callback_function
    );

    void detour_or_throw(
        const HMODULE& module_handle,
        const std::string& function_name,
        uintptr_t callback_function
    );

    void detour_or_warn(
        uintptr_t address,
        const std::string& function_name,
        uintptr_t callback_function
    );

    void detour_or_warn(
        const HMODULE& module_handle,
        const std::string& function_name,
        uintptr_t callback_function
    );

    void detour(
        uintptr_t address,
        const std::string& function_name,
        uintptr_t callback_function
    );

    void detour(
        const HMODULE& module_handle,
        const std::string& function_name,
        uintptr_t callback_function
    );

    void eat_hook_or_throw(
        const HMODULE& module_handle,
        const std::string& function_name,
        uintptr_t callback_function
    );

    void eat_hook_or_warn(
        const HMODULE& module_handle,
        const std::string& function_name,
        uintptr_t callback_function
    );

    void swap_virtual_func_or_throw(
        const void* instance,
        const std::string& function_name,
        uint16_t ordinal,
        uintptr_t callback_function
    );

    void swap_virtual_func(
        const void* instance,
        const std::string& function_name,
        uint16_t ordinal,
        uintptr_t callback_function
    );

    /**
     * @return address of the exported function in the given module.
     */
    uintptr_t get_module_function_address(
        const HMODULE& module_handle,
        const std::string& function_name
    );

    template<typename F>
    F get_module_function(const HMODULE& module_handle, const std::string& function_name, F) {
        return reinterpret_cast<F>(get_module_function_address(module_handle, function_name));
    }

    /**
     * @return address of the function that was hooked.
     */
    uintptr_t get_hooked_function_address(const std::string& function_name);

    template<typename F>
    F get_hooked_function(const std::string& function_name, F) {
        return reinterpret_cast<F>(get_hooked_function_address(function_name));
    }

    void init(bool print_info = false);

    bool is_hook_mode(const HMODULE& self_module, const std::string& orig_library_name);
}
