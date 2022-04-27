#pragma once

#include <koalabox/koalabox.hpp>
#include <koalabox/util.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

namespace koalabox::config_parser {

    /**
     * Parses the given json file and initializes the provided Config type with it
     */
    template<typename Config>
    Config parse(const Path& path, const bool enable_comments = false) {
        if (not exists(path)) {
            return Config{};
        }

        try {
            const auto json = nlohmann::json::parse(std::ifstream(path), nullptr, true, enable_comments);

            return json.get<Config>();
        } catch (const std::exception& ex) {
            util::panic("Failed to parse config file: {}", ex.what());
        }
    }

}
