#pragma once

#include "koalabox/koalabox.hpp"
#include "koalabox/util/util.hpp"

#include "3rd_party/json.hpp"

namespace koalabox::config_parser {

    template<typename C>
    C parse(Path path, const bool enable_comments = false) {
        if (not exists(path)) {
            return {};
        }

        try {
            std::ifstream ifs(std::move(path)); // Cannot be const
            const auto json = nlohmann::json::parse(ifs, nullptr, true, enable_comments);

            return json.get<C>();
        } catch (const std::exception& ex) {
            util::panic("Failed to parse config file: {}", ex.what());
        }
    }

}
