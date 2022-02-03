#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace koalabox {
    typedef std::filesystem::path Path;
    typedef std::string String;
    typedef std::wstring WideString;

    template<typename T>
    using Vector [[maybe_unused]] = std::vector<T>;

    [[maybe_unused]]
    extern String project_name;
}
