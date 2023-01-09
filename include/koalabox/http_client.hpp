#pragma once

#include <koalabox/core.hpp>

namespace koalabox::http_client {

    KOALABOX_API(Json) fetch_json(const String& url);

}
