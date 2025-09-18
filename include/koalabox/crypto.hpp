#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace koalabox::crypto {
    namespace fs = std::filesystem;

    // TODO: Move this somewhere else and delete this namespace
    std::vector<uint8_t> decode_hex_string(const std::string& hex_str);
}