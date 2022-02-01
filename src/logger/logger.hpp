#pragma once

#include "koalabox.hpp"

#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>

using namespace fmt::literals; // "{}"_format() helper

namespace koalabox::logger {

    typedef std::shared_ptr<spdlog::logger> Logger;

    extern Logger _instance;

    [[maybe_unused]]
    void init(Path& path);

    template<typename... Args>
    [[maybe_unused]]
    void trace(fmt::format_string<Args...> fmt, Args&& ...args) {
        _instance->trace(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[maybe_unused]]
    void debug(fmt::format_string<Args...> fmt, Args&& ...args) {
        _instance->debug(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[maybe_unused]]
    void info(fmt::format_string<Args...> fmt, Args&& ...args) {
        _instance->info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[maybe_unused]]
    void warn(fmt::format_string<Args...> fmt, Args&& ...args) {
        _instance->warn(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[maybe_unused]]
    void error(fmt::format_string<Args...> fmt, Args&& ...args) {
        _instance->error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[maybe_unused]]
    void critical(fmt::format_string<Args...> fmt, Args&& ...args) {
        _instance->critical(fmt, std::forward<Args>(args)...);
    }

}
