#pragma once

#include "../koalabox.hpp"

#include <Windows.h>
#include <functional>

namespace dll_monitor {
    using namespace koalabox;

    [[maybe_unused]]
    HMODULE init(const String& target_dll, const std::function<void(HMODULE module)>& callback);
}
