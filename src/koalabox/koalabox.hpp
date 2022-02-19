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

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    extern std::shared_ptr<spdlog::logger> log;

}
