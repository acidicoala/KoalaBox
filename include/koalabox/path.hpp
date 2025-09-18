#pragma once

#include <filesystem>
#include <string>

#include "koalabox/str.hpp"

/**
 * Utilities for converting paths to and from  std::string & std::wstring
 */
namespace koalabox::path {
    std::filesystem::path from_wstr(const std::wstring& wstr);
    std::filesystem::path from_str(const std::string& str);
    std::wstring to_wstr(const std::filesystem::path& path);
    std::string to_str(const std::filesystem::path& path);
    str::platform_string to_platform_str(const std::filesystem::path& path);
}