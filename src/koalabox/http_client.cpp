#include <koalabox/http_client.hpp>
#include <koalabox/util.hpp>
#include <koalabox/logger.hpp>

#include <cpr/cpr.h>

namespace koalabox::http_client {

    KOALABOX_API(Json) get_json(const String& url) {
        LOG_DEBUG("GET {}", url)

        const auto res = cpr::Get(cpr::Url{ url });

        if (res.status_code != cpr::status::HTTP_OK) {
            throw util::exception(
                "Status code: {}, Error code: {},\nResponse headers:\n{}\nBody:\n{}",
                res.status_code, (int) res.error.code, res.raw_header, res.text
            );
        }

        LOG_TRACE("Response text: \n{}", res.text)

        return Json::parse(res.text);
    }

    KOALABOX_API(Json) post_json(const String& url, Json payload) {
        LOG_DEBUG("POST {}", url)

        const auto res = cpr::Post(
            cpr::Url{ url },
            cpr::Header{{ "content-type", "application/json" }},
            cpr::Body{ payload.dump() }
        );

        if (res.status_code != cpr::status::HTTP_OK) {
            throw util::exception(
                "Status code: {}, Error code: {},\nResponse headers:\n{}\nBody:\n{}",
                res.status_code, (int) res.error.code, res.raw_header, res.text
            );
        }

        LOG_TRACE("Response text: \n{}", res.text)

        return Json::parse(res.text);
    }

}
