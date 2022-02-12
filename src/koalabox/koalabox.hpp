#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <set>
#include <map>

namespace koalabox {
    typedef std::filesystem::path Path;
    typedef std::string String;
    typedef std::wstring WideString;

    template<typename T>
    using Vector [[maybe_unused]] = std::vector<T>;

    template<typename T>
    using Set [[maybe_unused]] = std::set<T>;

    template<typename K, typename V>
    using Map [[maybe_unused]] = std::map<K, V>;
}
