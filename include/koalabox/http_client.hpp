#pragma once

#include <koalabox/core.hpp>

namespace koalabox::http_client {

    KOALABOX_API(Json) get_json(const String& url);

    KOALABOX_API(Json) post_json(const String& url, Json json);

    KOALABOX_API(String) head_etag(const String& url);

    KOALABOX_API(String) download_file(const String& url, const Path& destination);

}
