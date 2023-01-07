#pragma once

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

template<class T> using Set = std::set<T>;
template<typename T> using Vector = std::vector<T>;
template<class K, class V> using Map = std::map<K, V>;
template<class Fn> using Function = std::function<Fn>;

#define CALL_ONCE(FUNC_BODY)                \
    static std::once_flag _flag;            \
    std::call_once(_flag, [&]() FUNC_BODY);
