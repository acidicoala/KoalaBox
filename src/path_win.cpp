#include <utf8.h>

#include "koalabox/path.hpp"
#include "koalabox/str.hpp"

namespace koalabox::path {
    fs::path from_wstr(const std::wstring& wstr) {
        const auto u16str = str::to_u16str(wstr);
        const auto u8str = utf8::utf16tou8(u16str);
        return {u8str};
    }

    std::wstring to_wstr(const fs::path& path) {
        const auto u8str = path.generic_u8string();
        const auto u16str = utf8::utf8to16(u8str);
        return str::to_wstr(u16str);
    }

    string to_kb_str(const fs::path& path) {
        return to_wstr(path);
    }
}
