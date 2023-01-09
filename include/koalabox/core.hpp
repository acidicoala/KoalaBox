#pragma once

#include <filesystem>
#include <set>
#include <map>
#include <mutex>
#include <functional>

#define NOMINMAX
#include <minwindef.h>

#include <nlohmann/json.hpp>

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

constexpr auto BITNESS = 8 * sizeof(void*);

// Be warned, lurker. Dark sorcery awaits ahead of you...

/**
 * Performs case-insensitive string comparison. Usage: string1 < equals > string2
 * Source: https://stackoverflow.com/a/30145780
 * @return `true` if strings are equal, `false` otherwise
 */

// Useful for operators that don't modify operands
template<typename T, typename U>
struct ConstOperatorProxy {
private:
    const T& lhs;
    const U op;
public:
    explicit ConstOperatorProxy(const T& lhs, const U& op) : lhs(lhs), op(op) {}

    const T& operator*() const { return lhs; }
};


// Useful for operators that modify first operand
template<typename T, typename U>
struct MutableOperatorProxy {
private:
    T& lhs;
    const U op;
public:
    explicit MutableOperatorProxy(T& lhs, const U& op) : lhs(lhs), op(op) {}

    T& operator*() const { return lhs; }
};

/// String operators

#define DEFINE_CONST_OPERATOR(TYPE, OP) \
const struct OP##_t {} OP; \
bool operator>(const ConstOperatorProxy<TYPE, OP##_t>& lhs, const TYPE& rhs); \
template<typename T> \
ConstOperatorProxy<T, OP##_t> operator<(const T& lhs, const OP##_t& op) { \
    return ConstOperatorProxy<T, OP##_t>(lhs, op); \
}

DEFINE_CONST_OPERATOR(String, equals)
DEFINE_CONST_OPERATOR(String, not_equals)
DEFINE_CONST_OPERATOR(String, contains)

/// Vector operators

#define DEFINE_TEMPLATED_OPERATOR(TYPE, OP) \
const struct OP##_t {} OP; \
template<typename T> \
MutableOperatorProxy<T, OP##_t> operator<(T& lhs, const OP##_t& op) { \
    return MutableOperatorProxy<T, OP##_t>(lhs, op); \
} \
template<typename T> \
void operator>(const MutableOperatorProxy<TYPE<T>, OP##_t>& lhs, const TYPE<T>& rhs) { \
    (*lhs).insert((*lhs).end(), rhs.begin(), rhs.end()); \
}

DEFINE_TEMPLATED_OPERATOR(Vector, append)
