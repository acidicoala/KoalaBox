#pragma once

#include <koalabox/core.hpp>

/**
 * DLL Monitor starts a DLL load listener and calls the provided callback function
 * to notify of the load events that match the provided library name(s).
 */
namespace koalabox::dll_monitor {

    KOALABOX_API(void) init_listener(
        const Vector<String>& target_library_names,
        const Function<void(const HMODULE& module_handle, const String& library_name)>& callback
    );

    KOALABOX_API(void) init_listener(
        const String& target_library_name,
        const Function<void(const HMODULE& module_handle)>& callback
    );

    KOALABOX_API(void) shutdown_listener();

}
