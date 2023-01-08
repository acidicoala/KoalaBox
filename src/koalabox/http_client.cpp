#include <koalabox/http_client.hpp>
#include <koalabox/util.hpp>
#include <koalabox/logger.hpp>

#include <cpr/cpr.h>

namespace koalabox::http_client {

    KOALABOX_API(Json) fetch_json(const String& url) {
        LOG_DEBUG("GET {}", url)

        const auto res = cpr::Get(cpr::Url{url});

        if (res.status_code != cpr::status::HTTP_OK) {
            throw util::exception(
                "Status code: {}, Error code: {},\nResponse headers:\n{}\nBody:\n{}",
                res.status_code, (int) res.error.code, res.raw_header, res.text
            );
        }

        return Json(res.text);
    }

}
