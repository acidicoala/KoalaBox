#pragma once

namespace koalabox {
    using Path = std::filesystem::path;
    using String = std::string;
    using WideString = std::wstring;

    template<typename T>
    using Vector [[maybe_unused]] = std::vector<T>;

    template<typename T>
    using Set [[maybe_unused]] = std::set<T>;

    template<typename K, typename V>
    using Map [[maybe_unused]] = std::map<K, V>;
}
