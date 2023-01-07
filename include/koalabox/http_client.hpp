#pragma once

#include <koalabox/types.hpp>
#include <nlohmann/json.hpp>

namespace koalabox::http_client {

    nlohmann::json fetch_json(const String& url);

}
