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
            const auto config_str = io::read_file(config_path);

            const auto config = Json::parse(config_str).get<Config>();

            LOG_DEBUG("Parsed config:\n{}", Json(config).dump(2));

            return config;
        } catch (const Exception& e) {
            util::panic("Error parsing config file: {}", e.what());
        }
    }

    template<class Config>
    KOALABOX_API(Config) parse() {
        return parse<Config>(paths::get_config_path());
    }
}
