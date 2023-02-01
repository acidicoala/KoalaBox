#pragma once

#include <koalabox/core.hpp>

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <httplib.h>

namespace koalabox::http_server {

    constexpr auto CONTENT_TYPE_JSON = "application/json";

    void start(
        const String& local_host,
        const String& server_host,
        unsigned int port,
        const Map<String, httplib::Server::Handler>& pattern_handlers
    ) noexcept;

}
