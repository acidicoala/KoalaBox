#pragma once

#include <nlohmann/json.hpp>

/**
 * This namespace contains utility functions for reading from and writing to cache file on disk.
 * All functions are intended to be safe to call, i.e. they should not throw exceptions.
 */
namespace koalabox::cache {

    nlohmann::json get(const std::string& key, const nlohmann::json& fallback = nlohmann::json());

    bool put(const std::string& key, const nlohmann::json& value) noexcept;

}
