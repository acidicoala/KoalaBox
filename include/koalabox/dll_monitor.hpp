#pragma once

#include <koalabox/core.hpp>

/**
 * DLL Monitor starts a DLL load listener and calls the provided callback function
 * to notify of the load events that match the provided library name(s).
 */
namespace koalabox::dll_monitor {

    void init_listener(
        const std::vector<std::string>& target_library_names,
        const std::function<void( //
            const HMODULE& module_handle,
            const std::string& library_name
        )>& callback
    );

    void init_listener(
        const std::string& target_library_name,
        const std::function<void(const HMODULE& module_handle)>& callback
    );

    void shutdown_listener();

}
