#pragma once

#include <functional>
#include <string>
#include <vector>

/**
 * DLL Monitor starts a DLL load listener and calls the provided callback function
 * to notify of the load events that match the provided library name(s).
 */
namespace koalabox::dll_monitor {
    using callback_t = void(const HMODULE& module_handle);
    using callback_multi_t = void(const HMODULE& module_handle, const std::string& library_name);

    /**
     * Invokes the callback when DLL with target_library_name is loaded
     */
    void init_listener(
        const std::string& target_library_name,
        const std::function<callback_t>& callback
    );

    /**
     * Invokes the callback when DLL matching one of target_library_names is loaded
     */
    void init_listener(
        const std::vector<std::string>& target_library_names,
        const std::function<callback_multi_t>& callback
    );

    void shutdown_listener();
}