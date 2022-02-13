#pragma once

// Windows headers
#define WIN32_LEAN_AND_MEAN
#define UNICODE

#include <Windows.h>
#include <Psapi.h>

// C++ Standard Library
#include <filesystem>// std::filesystem
#include <utility>   // std::forward
#include <functional>// std::function
#include <fstream>   // std::ifstream
#include <regex>     // std::regex
#include <set>       // std::set
#include <map>       // std::map
#include <memory>    // std::shared_ptr
#include <string>    // std::string | std::wstring
#include <thread>    // std::thread
#include <vector>    // std::vector

// Spdlog headers
#include <spdlog/logger.h>               // spdlog::logger
#include <spdlog/pattern_formatter.h>    // spdlog::custom_flag_formatter
#include <spdlog/sinks/basic_file_sink.h>// spdlog::basic_logger_mt
#include <spdlog/sinks/null_sink.h>      // spdlog::null_logger_mt
