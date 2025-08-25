#pragma once

#include <functional>
#include <set>
#include <string>

/**
 * DLL Monitor starts a DLL load listener and calls the provided callback function
 * to notify of the load events that match the provided library name(s).
 */
namespace koalabox::dll_monitor {
    using callback_multi_t = void(const HMODULE& module_handle, const std::string& library_name);

    /**
     * Invokes the callback when DLL matching one of target_library_names is loaded
     */
    void init_listener(
        const std::set<std::string>& target_library_names,
        const std::function<callback_multi_t>& callback
    );
}