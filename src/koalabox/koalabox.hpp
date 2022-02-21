#pragma once

namespace koalabox {

    using Path = std::filesystem::path;
    using String = std::string;
    using WideString = std::wstring;
    using Exception = std::exception;
    using FunctionPointer = char*;

    template<typename T>
    using Vector = std::vector<T>;

    template<typename T>
    using Set = std::set<T>;

    template<typename K, typename V>
    using Map = std::map<K, V>;

    extern std::shared_ptr<spdlog::logger> logger;

}
