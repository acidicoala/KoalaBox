#include "koalabox/util.hpp"


namespace koalabox::util {
    std::optional<std::string> get_env(const std::string& key) noexcept {
        // TODO: Replace with non-deprecated version
        if(const auto* value = std::getenv(key.c_str())) {
            return value;
        }

        return {};
    }
}
