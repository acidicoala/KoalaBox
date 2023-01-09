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

// Be warned, lurker. Dark sorcery await ahead of you...

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

// TODO: Macro definitions

/// String operators

/// < equals >

const struct equals_t {} equals;

template<typename T>
ConstOperatorProxy<T, equals_t> operator<(const T& lhs, const equals_t& op) {
    return ConstOperatorProxy<T, equals_t>(lhs, op);
}

bool operator>(const ConstOperatorProxy<String, equals_t>& lhs, const String& rhs);

/// < not_equals >
const struct not_equals_t {} not_equals;

template<typename T>
ConstOperatorProxy<T, not_equals_t> operator<(const T& lhs, const not_equals_t& op) {
    return ConstOperatorProxy<T, not_equals_t>(lhs, op);
}

bool operator>(const ConstOperatorProxy<String, not_equals_t>& lhs, const String& rhs);

/// < contains >
const struct contains_t {} contains;

template<typename T>
ConstOperatorProxy<T, contains_t> operator<(const T& lhs, const contains_t& op) {
    return ConstOperatorProxy<T, contains_t>(lhs, op);
}

bool operator>(const ConstOperatorProxy<String, contains_t>& lhs, const String& rhs);

/// Vector operators

const struct append_t {} append;

template<typename T>
MutableOperatorProxy<T, append_t> operator<(T& lhs, const append_t& op) {
    return MutableOperatorProxy<T, append_t>(lhs, op);
}

template<typename VT>
void operator>(const MutableOperatorProxy<Vector<VT>, append_t>& lhs, const Vector<VT>& rhs) {
    (*lhs).insert((*lhs).end(), rhs.begin(), rhs.end());
}
