#include <koalabox/http_client.hpp>
#include <koalabox/util.hpp>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

namespace koalabox::http_client {
    nlohmann::json fetch_json(const String& url) {
        const auto res = cpr::Get(cpr::Url{url});

        if (res.status_code != cpr::status::HTTP_OK) {
            throw util::exception(
                "Status code: {}, Error code: {},\nResponse headers:\n{}\nBody:\n{}",
                res.status_code, (int) res.error.code, res.raw_header, res.text
            );
        }

        return nlohmann::json::parse(res.text);
    }
}
