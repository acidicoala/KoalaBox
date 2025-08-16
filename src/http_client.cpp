#include <koalabox/http_client.hpp>
#include <koalabox/util.hpp>
#include <koalabox/logger.hpp>

#include <cpr/cpr.h>


namespace koalabox::http_client {
    namespace {
        void validate_ok_response(const cpr::Response& res) {
            if (res.status_code != cpr::status::HTTP_OK) {
                throw util::exception(
                    "Status code: {}, Error code: {},\nResponse headers:\n{}\nBody:\n{}",
                    res.status_code, (int)res.error.code, res.raw_header, res.text
                );
            }
        }
    }

    KOALABOX_API(Json) get_json(const String& url) {
        LOG_DEBUG("GET {}", url);

        const auto res = cpr::Get(cpr::Url{url});

        validate_ok_response(res);

        LOG_TRACE("Response text: \n{}", res.text);

        return Json::parse(res.text);
    }

    KOALABOX_API(Json) post_json(const String& url, Json payload) {
        LOG_DEBUG("POST {}", url);

        const auto res = cpr::Post(
            cpr::Url{url},
            cpr::Header{{"content-type", "application/json"}},
            cpr::Body{payload.dump()}
        );

        validate_ok_response(res);

        LOG_TRACE("Response text: \n{}", res.text);

        return Json::parse(res.text);
    }

    KOALABOX_API(String) head_etag(const String& url) {
        const auto res = cpr::Head(cpr::Url{url});

        validate_ok_response(res);

        if (res.header.find("etag") != res.header.end()) {
            const auto etag = res.header.at("etag");
            LOG_TRACE(R"(Etag for url "{}" = "{}")", url, etag);
            return etag;
        }

        LOG_TRACE(R"(No etag found for url "{}")", url);

        return "";
    }

    KOALABOX_API(String) download_file(const String& url, const Path& destination) {
        LOG_DEBUG(R"(Downloading "{}" to "{}")", url, destination.string());

        std::ofstream of(destination, std::ios::binary);
        cpr::Response res = cpr::Download(of, cpr::Url{url});

        validate_ok_response(res);

        LOG_DEBUG("Download complete ({} bytes)", res.downloaded_bytes);

        if (res.header.find("etag") != res.header.end()) {
            return res.header.at("etag");
        }

        return "";
    }
}