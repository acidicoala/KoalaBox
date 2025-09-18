#include "koalabox/path.hpp"

namespace koalabox::path {
    std::filesystem::path from_str(const std::string& str) {
        const auto u8str = std::u8string(str.begin(), str.end());
        return {u8str};
    }

    std::string to_str(const std::filesystem::path& path) {
        const auto u8str = path.generic_u8string();
        return {u8str.begin(), u8str.end()};
    }
}
