#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace koalabox::crypto {
    namespace fs = std::filesystem;

    std::vector<uint8_t> decode_hex_string(const std::string& hex_str);

    std::string calculate_md5(const fs::path& file_path);
}
