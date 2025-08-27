#pragma once

#include <nlohmann/json.hpp>

#include "koalabox/io.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/paths.hpp"
#include "koalabox/util.hpp"

namespace koalabox::config {
    namespace fs = std::filesystem;

    template<class Config>
    Config parse(const fs::path& config_path) {
        if(not fs::exists(config_path)) {
            return Config();
        }

        try {
            const auto config_str = io::read_file(config_path);

            const auto config = nlohmann::json::parse(config_str).get<Config>();

            return config;
        } catch(const std::exception& e) {
            util::panic(std::format("Error parsing config file: {}", e.what()));
        }
    }

    template<class Config>
    Config parse() {
        return parse<Config>(paths::get_config_path());
    }
}