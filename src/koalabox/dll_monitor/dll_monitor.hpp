#pragma once

#include "koalabox/koalabox.hpp"

namespace koalabox::dll_monitor {

    void init(const String& target_library_name, const std::function<void(HMODULE module)>& callback);

    void shutdown();

}
