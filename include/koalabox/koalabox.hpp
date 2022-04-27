#pragma once

#include <filesystem>
#include <set>
#include <map>

#include <spdlog/logger.h>

namespace koalabox {

    using Path = std::filesystem::path;
    using String = std::string;
    using WideString = std::wstring;
    using Exception = std::exception;
    using FunctionAddress = uint64_t;

    template<typename T>
    using Vector = std::vector<T>;

    template<typename T>
    using Set = std::set<T>;

    template<typename K, typename V>
    using Map = std::map<K, V>;

    extern std::shared_ptr<spdlog::logger> logger;

    extern String project_name;

}
