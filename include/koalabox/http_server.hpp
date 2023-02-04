#pragma once

#include <koalabox/core.hpp>

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <httplib.h>

namespace koalabox::http_server {

    constexpr auto CONTENT_TYPE_JSON = "application/json";

    void start_proxy_server(
        const String& original_server_host,
        unsigned int original_server_port,
        const String& proxy_server_host,
        unsigned int proxy_server_port,
        const String& port_proxy_ip,
        const Map<String, httplib::Server::Handler>& pattern_handlers
    ) noexcept;

}
