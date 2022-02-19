#pragma once

// Windows headers
#define WIN32_LEAN_AND_MEAN
#define UNICODE

#include <Windows.h>

// Process Status API must be included after windows
#include <Psapi.h>

// C++ Standard Library
#include <filesystem> // std::filesystem
#include <fstream>    // std::ifstream
#include <functional> // std::function
#include <map>        // std::map
#include <memory>     // std::shared_ptr | std::unique_ptr
#include <regex>      // std::regex
#include <set>        // std::set
#include <string>     // std::string | std::wstring
#include <thread>     // std::thread
#include <utility>    // std::forward
#include <vector>     // std::vector

#include "3rd_party/spdlog.hpp"
