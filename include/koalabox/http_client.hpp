#pragma once

#include <koalabox/core.hpp>
#include <koalabox/json.hpp>

namespace koalabox::http_client {

    KOALABOX_API(Json) fetch_json(const String& url);

}
