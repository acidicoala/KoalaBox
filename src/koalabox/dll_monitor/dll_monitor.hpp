#pragma once

#include "../koalabox.hpp"

namespace dll_monitor {
    using namespace koalabox;

    [[maybe_unused]]
    void init(
        const String& target_library_name,
        const std::function<void(HMODULE module)>& callback
    );

    [[maybe_unused]]
    void shutdown();
}
