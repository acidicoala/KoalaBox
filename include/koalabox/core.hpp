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

#define NEW_THREAD(FUNC_BODY) \
    std::thread( \
        [=]() FUNC_BODY \
    ).detach();

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

// TODO: Replace with normal function
DEFINE_TEMPLATED_OPERATOR(Vector, append)
