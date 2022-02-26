#pragma once

#include "koalabox/koalabox.hpp"

/**
 * DLL Monitor starts a DLL load listener and calls the provided callback function
 * to notify of the load events that match the provided library name(s).
 */
namespace koalabox::dll_monitor {

    void init(
        const Vector <String>& target_library_names,
        const std::function<void(const HMODULE& module, const String& library_name)>& callback
    );

    void init(
        const String& target_library_name,
        const std::function<void(const HMODULE& module)>& callback
    );

    void shutdown();

}
