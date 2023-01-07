#pragma once

#include <koalabox/types.hpp>

/**
 * DLL Monitor starts a DLL load listener and calls the provided callback function
 * to notify of the load events that match the provided library name(s).
 */
namespace koalabox::dll_monitor {

    void init(
        const Vector <String>& target_library_names,
        const Function<void(const HMODULE& module_handle, const String& library_name)>& callback
    );

    void init(
        const String& target_library_name,
        const Function<void(const HMODULE& module_handle)>& callback
    );

    void shutdown();

}
