#pragma once

#include <koalabox/core.hpp>
#include <koalabox/io.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/paths.hpp>
#include <koalabox/util.hpp>

namespace koalabox::config {

    template<class Config>
    KOALABOX_API(Config) parse(Path config_path) {
        if (not exists(config_path)) {
            return Config();
        }

        try {
            const auto config_str = koalabox::io::read_file(config_path);

            const auto config = Json::parse(config_str).get<Config>();

            LOG_DEBUG("Parsed config:\n{}", Json(config).dump(2))

            return config;
        } catch (const Exception& e) {
            const auto message = fmt::format("Error parsing config file: {}", e.what());
            koalabox::util::error_box("SmokeAPI Error", message);
        }
    }

    template<class Config>
    KOALABOX_API(Config) parse() {
        return parse<Config>(koalabox::paths::get_config_path());
    }
}
