#pragma once

#include <koalabox/types.hpp>
#include <spdlog/spdlog.h>

namespace koalabox::logger {
    /**
     * A global single instance of the logger. Not meant to be used directly,
     * as it is considered implementation detail. Instead, callers should use
     * the macros defined below.
     */
    extern std::shared_ptr<spdlog::logger> instance;

    void init_file_logger(const Path& path);
}

// Define trace in a special way to avoid excessive logging in release builds
#ifdef _DEBUG
#define LOG_TRACE(fmt, ...) koalabox::logger::instance->trace(fmt, __VA_ARGS__);
#else
#define LOG_TRACE(...)
#endif

#define LOG_DEBUG(fmt, ...) koalabox::logger::instance->debug(fmt, __VA_ARGS__);
#define LOG_WARN(fmt, ...) koalabox::logger::instance->warn(fmt, __VA_ARGS__);
#define LOG_INFO(fmt, ...) koalabox::logger::instance->info(fmt, __VA_ARGS__);
#define LOG_ERROR(fmt, ...) koalabox::logger::instance->error(fmt, __VA_ARGS__);
#define LOG_CRITICAL(fmt, ...) koalabox::logger::instance->critical(fmt, __VA_ARGS__);
