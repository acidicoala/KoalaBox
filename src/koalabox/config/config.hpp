#pragma once

#include "koalabox/koalabox.hpp"
#include "koalabox/util/util.hpp"

#include "3rd_party/json.hpp"

namespace koalabox::config {

    template<typename C>
    C parse(const Path& path) {
        if (not exists(path)) {
            return {};
        }

        try {
            const std::ifstream ifs(path);
            const auto json = nlohmann::json::parse(ifs, nullptr, true, true);

            return json.get<C>();
        } catch (const std::exception& ex) {
            util::panic("Failed to parse config file: {}", ex.what());
        }
    }

}
