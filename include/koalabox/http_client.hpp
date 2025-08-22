#pragma once

#include <nlohmann/json.hpp>

namespace koalabox::http_client {
    namespace fs = std::filesystem;

    nlohmann::json get_json(const std::string& url);

    nlohmann::json post_json(const std::string& url, nlohmann::json json);

    std::string head_etag(const std::string& url);

    std::string download_file(const std::string& url, const fs::path& destination);
}