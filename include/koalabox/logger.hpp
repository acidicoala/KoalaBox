#pragma once

#include <filesystem>

#include <spdlog/spdlog.h>

namespace koalabox::logger {
    void init_file_logger(const std::filesystem::path& log_path);
    void init_console_logger();
    void init_null_logger();

    void shutdown();
}

// Use macros for logging to capture source file, line number and function name

#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_ERROR(...) \
    try { SPDLOG_ERROR(__VA_ARGS__) ; } \
    catch (...) { OutputDebugString(TEXT("Exception printing error log")); DebugBreak(); }
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)
