#include <cpr/cpr.h>

#include "koalabox/http_client.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

namespace koalabox::http_client {
    namespace {
        void validate_ok_response(const cpr::Response& res) {
            if(res.status_code != cpr::status::HTTP_OK) {
                LOG_TRACE("Response headers:\n{}", res.raw_header);
                LOG_TRACE("Response body:\n{}", res.text);

                throw std::runtime_error(
                    std::format(
                        "Status code: {}, Error code: {}",
                        res.status_code, static_cast<int>(res.error.code)
                    )
                );
            }
        }
    }

    nlohmann::json get_json(const std::string& url) {
        LOG_DEBUG("GET {}", url);

        const auto res = cpr::Get(cpr::Url{url});

        validate_ok_response(res);

        LOG_TRACE("Response text: \n{}", res.text);

        return nlohmann::json::parse(res.text);
    }

    nlohmann::json post_json(
        const std::string& url,
        const nlohmann::json& payload,
        const std::map<std::string, std::string>& headers
    ) {
        LOG_DEBUG("POST {}", url);

        auto cpr_headers = cpr::Header{{"content-type", "application/json"}};
        cpr_headers.insert(headers.begin(), headers.end());

        const auto res = cpr::Post(
            cpr::Url{url},
            cpr_headers,
            cpr::Body{payload.dump()}
        );

        validate_ok_response(res);

        LOG_TRACE("Response text: \n{}", res.text);

        return nlohmann::json::parse(res.text);
    }

    std::string head_etag(const std::string& url) {
        const auto res = cpr::Head(cpr::Url{url});

        validate_ok_response(res);

        if(res.header.contains("etag")) {
            const auto etag = res.header.at("etag");
            LOG_TRACE(R"(Etag for url "{}" = "{}")", url, etag);
            return etag;
        }

        LOG_TRACE(R"(No etag found for url "{}")", url);

        return "";
    }

    std::string download_file(const std::string& url, const fs::path& destination) {
        LOG_DEBUG(R"(Downloading "{}" to "{}")", url, path::to_str(destination));

        std::ofstream of(destination, std::ios::binary);
        cpr::Response res = cpr::Download(of, cpr::Url{url});

        validate_ok_response(res);

        LOG_DEBUG("Download complete ({} bytes)", res.downloaded_bytes);

        if(res.header.contains("etag")) {
            return res.header.at("etag");
        }

        return "";
    }
}
