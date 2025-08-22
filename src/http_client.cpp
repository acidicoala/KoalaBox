#include <cpr/cpr.h>

#include "koalabox/http_client.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/util.hpp"

namespace koalabox::http_client {
    namespace {
        void validate_ok_response(const cpr::Response& res) {
            if (res.status_code != cpr::status::HTTP_OK) {
                throw std::runtime_error(
                    std::format(
                        "Status code: {}, Error code: {},\nResponse headers:\n{}\nBody:\n{}",
                        res.status_code, static_cast<int>(res.error.code), res.raw_header, res.text
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

    nlohmann::json post_json(const std::string& url, const nlohmann::json& payload) {
        LOG_DEBUG("POST {}", url);

        const auto res = cpr::Post(
            cpr::Url{url}, cpr::Header{{"content-type", "application/json"}},
            cpr::Body{payload.dump()}
        );

        validate_ok_response(res);

        LOG_TRACE("Response text: \n{}", res.text);

        return nlohmann::json::parse(res.text);
    }

    std::string head_etag(const std::string& url) {
        const auto res = cpr::Head(cpr::Url{url});

        validate_ok_response(res);

        if (res.header.contains("etag")) {
            const auto etag = res.header.at("etag");
            LOG_TRACE(R"(Etag for url "{}" = "{}")", url, etag);
            return etag;
        }

        LOG_TRACE(R"(No etag found for url "{}")", url);

        return "";
    }

    std::string download_file(const std::string& url, const fs::path& destination) {
        LOG_DEBUG(R"(Downloading "{}" to "{}")", url, destination.string());

        std::ofstream of(destination, std::ios::binary);
        cpr::Response res = cpr::Download(of, cpr::Url{url});

        validate_ok_response(res);

        LOG_DEBUG("Download complete ({} bytes)", res.downloaded_bytes);

        if (res.header.contains("etag")) {
            return res.header.at("etag");
        }

        return "";
    }
}