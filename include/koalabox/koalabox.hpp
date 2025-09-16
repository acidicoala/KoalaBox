#pragma once

#include <optional>
#include <stdexcept>
#include <string>

namespace koalabox {
    struct throw_if_empty {
    private:
        std::string message;

    public:
        explicit throw_if_empty(std::string msg) : message(std::move(msg)) {}

        template<typename T>
        T operator()(std::optional<T> opt_value) const {
            if(!opt_value) {
                throw std::runtime_error(message);
            }
            return *opt_value;
        }
    };

    // Overload the pipe operator
    template<typename T, typename F>
    auto operator|(T&& value, F&& func) -> decltype(func(std::forward<T>(value))) {
        return func(std::forward<T>(value));
    }

    // Platform-specific type aliases
#ifdef KB_WIN
    using string = std::wstring;
#elifdef KB_LINUX
    using string = std::string;
#endif
}
