#pragma once

namespace koalabox::platform {
    constexpr uint8_t bitness = 8 * sizeof(void*);

    constexpr bool is_32bit = sizeof(void*) == 4;
    constexpr bool is_64bit = !is_32bit;

#ifdef KB_WIN
    constexpr bool is_windows = true;
#else
    constexpr bool is_windows = false;
#endif
    constexpr bool is_linux = !is_windows;
}
