#pragma once

#include <koalabox/core.hpp>
#include <koalabox/util.hpp>
#include <koalabox/io.hpp>

namespace koalabox::config_parser {

    /**
     * Parses the given json file and initializes the provided Config type with it
     */
    template<typename Config>
    KOALABOX_API(Config) parse(const Path& path) {
        try {
            const auto file_opt = io::read_file(path);

            return Json::parse(file_opt.value()).get<Config>();
        } catch (const std::exception& ex) {
            util::panic("Failed to parse config file: {}", ex.what());
        }
    }

}
