#pragma once

#include <filesystem>
#include <string>

namespace koalabox {
    typedef std::filesystem::path Path;
    typedef std::string String;
    typedef std::wstring WideString;

    [[maybe_unused]]
    extern String project_name;
}
