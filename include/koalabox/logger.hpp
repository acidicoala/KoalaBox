#pragma once

#include <koalabox/core.hpp>

#include <spdlog/spdlog.h>


namespace koalabox::logger {
    /**
     * A global single instance of the logger. Not meant to be used directly,
     * as it is considered implementation detail. Instead, callers should use
     * the macros defined below.
     */
    extern std::shared_ptr<spdlog::logger> instance;

    KOALABOX_API(void) init_file_logger(const Path& path);

    KOALABOX_API(void) init_file_logger();

    KOALABOX_API(String) get_filename(const char* full_path);
}

#define LOG_MESSAGE(LEVEL, fmt, ...) koalabox::logger::instance->LEVEL( \
    " {:>3}:{:24} ┃ " fmt, __LINE__, koalabox::logger::get_filename(__FILE__) , __VA_ARGS__ \
);

// Define trace in a special way to avoid excessive logging in release builds
#ifdef _DEBUG
#define LOG_TRACE(fmt, ...) LOG_MESSAGE(trace, fmt, __VA_ARGS__)
#else
#define LOG_TRACE(...)
#endif

#define LOG_DEBUG(fmt, ...)     LOG_MESSAGE(debug, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...)      LOG_MESSAGE(warn, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...)      LOG_MESSAGE(info, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...)     LOG_MESSAGE(error, fmt, __VA_ARGS__)
#define LOG_CRITICAL(fmt, ...)  LOG_MESSAGE(critical, fmt, __VA_ARGS__)
