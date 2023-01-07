#pragma once

// TODO: Extract into parser package
#include <nlohmann/json.hpp>

#include <filesystem>
#include <set>
#include <map>
#include <mutex>
#include <functional>

using Mutex = std::mutex;
using MutexLockGuard = std::lock_guard<Mutex>;
using String = std::string;
using WideString = std::wstring;
using Exception = std::exception;
using Path = std::filesystem::path;
using Json = nlohmann::json;

template<class T> using Set = std::set<T>;
template<typename T> using Vector = std::vector<T>;
template<class K, class V> using Map = std::map<K, V>;
template<class Fn> using Function = std::function<Fn>;

#define NOMINMAX
#include <minwindef.h>

#define SUPPRESS_UNUSED(PARAM) (void) PARAM;
#define KOALABOX_API(...) [[maybe_unused]] __VA_ARGS__

constexpr auto BITNESS = 4 * sizeof(void*);

#define CALL_ONCE(FUNC_BODY) \
    static std::once_flag _flag; \
    std::call_once(_flag, [&]() FUNC_BODY);
