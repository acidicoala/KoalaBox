#pragma once

#include <koalabox/core.hpp>

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <httplib.h>

namespace koalabox::http_server {

    constexpr auto CONTENT_TYPE_JSON = "application/json";

    void start(
        const String& local_host,
        unsigned int port,
        const String& server_host,
        const Map<String, httplib::Server::Handler>& pattern_handlers
    ) noexcept;

}
