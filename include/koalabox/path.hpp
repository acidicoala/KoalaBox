#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

/**
 * Utilities for converting paths to and from std::wstring
 */
namespace koalabox::path {
    fs::path from_wstr(const std::wstring& wstr);
    std::wstring to_wstr(const fs::path& path);
    std::string to_str(const fs::path& path);
}