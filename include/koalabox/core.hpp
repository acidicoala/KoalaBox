#pragma once

#include <filesystem>
#include <set>
#include <map>
#include <mutex>
#include <functional>

#include <nlohmann/json.hpp>

#define SUPPRESS_UNUSED(PARAM) (void) PARAM;
#define KOALABOX_API(...) [[maybe_unused]] __VA_ARGS__

#define DECLARE_STRUCT(TYPE, VAR_NAME) \
    TYPE VAR_NAME = {}; \
    memset(&VAR_NAME, 0, sizeof(TYPE))

#define CALL_ONCE(FUNC_BODY) \
    static std::once_flag _flag; \
    std::call_once(_flag, [&]() FUNC_BODY);

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

constexpr auto BITNESS = 8 * sizeof(void*);
