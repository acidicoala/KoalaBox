#pragma once

// TODO: Extract into parser package
#include <nlohmann/json.hpp>

#include <filesystem>
#include <set>
#include <map>
#include <mutex>
#include <functional>

#define NOMINMAX
#include <minwindef.h>

#define SUPPRESS_UNUSED(PARAM) (void) PARAM;
#define KOALABOX_API(...) [[maybe_unused]] __VA_ARGS__

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

constexpr auto BITNESS = 4 * sizeof(void*);

// Be warned, lurker. Dark sorcery await ahead of you...

/**
 * Performs case-insensitive string comparison. Usage: string1 < equals > string2
 * Source: https://stackoverflow.com/a/30145780
 * @return `true` if strings are equal, `false` otherwise
 */
const struct equals_t {} equals;
const struct not_equals_t {} not_equals;
const struct contains_t {} contains;

template<typename T, typename U>
struct OperatorProxy {
private:
    const T& lhs;
    const U op;
public:
    explicit OperatorProxy(const T& lhs, const U& op) : lhs(lhs), op(op) {}

    const T& operator*() const { return lhs; }
};

/// < equals >

template<typename T>
OperatorProxy<T, equals_t> operator<(const T& lhs, const equals_t& op) {
    return OperatorProxy<T, equals_t>(lhs, op);
}

bool operator>(const OperatorProxy<String, equals_t>& lhs, const String& rhs);

/// < not_equals >

template<typename T>
OperatorProxy<T, not_equals_t> operator<(const T& lhs, const not_equals_t& op) {
    return OperatorProxy<T, not_equals_t>(lhs, op);
}

bool operator>(const OperatorProxy<String, not_equals_t>& lhs, const String& rhs);

/// < contains >

template<typename T>
OperatorProxy<T, contains_t> operator<(const T& lhs, const contains_t& op) {
    return OperatorProxy<T, contains_t>(lhs, op);
}

bool operator>(const OperatorProxy<String, contains_t>& lhs, const String& rhs);
