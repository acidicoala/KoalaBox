#pragma once

#include <string>

#define KB_HOOK_GET_HOOKED_FN(FUNC) koalabox::hook::get_hooked_function(#FUNC, FUNC)
#define KB_HOOK_GET_SWAPPED_FN(CLASS, FUNC) koalabox::hook::get_swapped_function(CLASS, #FUNC, FUNC)

#define KB_HOOK_DETOUR_ADDRESS(FUNC, ADDRESS) \
    koalabox::hook::detour_or_warn(ADDRESS, #FUNC, reinterpret_cast<void*>(FUNC))

#define KB_HOOK_DETOUR_MODULE(FUNC, MODULE) \
    koalabox::hook::detour_or_warn(MODULE, #FUNC, reinterpret_cast<void*>(FUNC))

namespace koalabox::hook {
    bool is_hooked(const std::string& function_name);
    bool is_vt_hooked(const void* class_ptr, const std::string& function_name);
    bool unhook(const std::string& function_name);
    bool unhook_vt(const void* class_ptr, const std::string& function_name);

    void detour_or_throw(
        const void* address,
        const std::string& function_name,
        const void* callback_function
    );

    void detour_or_throw(
        const HMODULE& module_handle,
        const std::string& function_name,
        const void* callback_function
    );

    void detour_or_warn(
        const void* address,
        const std::string& function_name,
        const void* callback_function
    );

    void detour_or_warn(
        const HMODULE& module_handle,
        const std::string& function_name,
        const void* callback_function
    );

    void detour(
        const void* address,
        const std::string& function_name,
        const void* callback_function
    );

    void detour(
        const HMODULE& module_handle,
        const std::string& function_name,
        const void* callback_function
    );

    void eat_hook_or_throw(
        const HMODULE& module_handle,
        const std::string& function_name,
        const void* callback_function
    );

    void eat_hook_or_warn(
        const HMODULE& module_handle,
        const std::string& function_name,
        const void* callback_function
    );

    void swap_virtual_func_or_throw(
        const void* class_ptr,
        const std::string& function_name,
        uint16_t ordinal,
        const void* callback_function
    );

    void swap_virtual_func(
        const void* class_ptr,
        const std::string& function_name,
        uint16_t ordinal,
        const void* callback_function
    );

    /**
     * @return address of the function that was hooked.
     */
    void* get_hooked_function_address(const std::string& function_name);

    template<typename F>
    F get_hooked_function(const std::string& function_name, F) {
        return reinterpret_cast<F>(get_hooked_function_address(function_name));
    }

    /**
     * @return address of the function that was hooked.
     */
    void* get_swapped_function_address(
        const void* class_ptr,
        const std::string& function_name
    );

    template<typename F>
    F get_swapped_function(const void* class_ptr, const std::string& function_name, F) {
        return reinterpret_cast<F>(get_swapped_function_address(class_ptr, function_name));
    }

    void init(bool print_info = false);

    bool is_hook_mode(const HMODULE& self_module, const std::string& orig_library_name);
}
