#pragma once

#include <koalabox/core.hpp>

#define CPPHTTPLIB_OPENSSL_SUPPORT

#include <httplib.h>

namespace koalabox::http_server {

    constexpr auto CONTENT_TYPE_JSON = "application/json";

    namespace dns {
        std::optional<String> get_real_ip(const String& hostname);

        std::optional<String> get_canonical_name(const String& hostname);
    }

    bool make_original_request(
        const String& bypass_server_host,
        httplib::Request req,
        httplib::Response& res
    );

    /**
     * NOTE: None of the hosts should contains `https://` schema
     */
    void start_proxy_server(
        const String& bypass_server_host,
        const String& original_server_host,
        unsigned int original_server_port,
        const String& proxy_server_host,
        unsigned int proxy_server_port,
        const String& port_proxy_ip,
        const Vector<std::pair<String, httplib::Server::Handler>>& pattern_handlers
    ) noexcept;

}
