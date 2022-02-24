#pragma once

#include "koalabox/koalabox.hpp"

namespace koalabox::dll_monitor {

    void init(
        const Vector<String>& target_library_names,
        const std::function<void(const HMODULE& module, const String& library_name)>& callback
    );

    void init(
        const String& target_library_name,
        const std::function<void(const HMODULE& module)>& callback
    );

    void shutdown();

}
