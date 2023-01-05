#pragma once

#include <koalabox/koalabox.hpp>
#include <nlohmann/json.hpp>

namespace koalabox::http_client {

    nlohmann::json fetch_json(const String& url);

}
