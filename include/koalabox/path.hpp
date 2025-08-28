#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

/**
 * Utilities for converting paths to and from  std::string & std::wstring
 */
namespace koalabox::path {
    fs::path from_wstr(const std::wstring& wstr);
    fs::path from_str(const std::string& str);
    std::wstring to_wstr(const fs::path& path);
    std::string to_str(const fs::path& path);
}