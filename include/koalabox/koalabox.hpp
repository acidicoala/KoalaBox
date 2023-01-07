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
#ifdef _WIN64
    using FunctionAddress = uint64_t;
#else
    using FunctionAddress = uint32_t;
#endif

    template<typename T>
    using Vector = std::vector<T>;

    template<typename T>
    using Set = std::set<T>;

    template<typename K, typename V>
    using Map = std::map<K, V>;

    extern std::shared_ptr<spdlog::logger> logger;

    extern String project_name;

}

#ifdef _DEBUG
#define TRACE(fmt, ...) koalabox::logger->trace(fmt, __VA_ARGS__);
#else
#define TRACE(...)
#endif
