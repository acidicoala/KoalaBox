#include <koalabox/cache.hpp>
#include <koalabox/io.hpp>
#include <koalabox/logger.hpp>

namespace koalabox::cache {

    namespace {
        Path cache_path;

        std::optional<Json> read_cache() {
            try {
                const auto cache = io::read_file(cache_path).value();

                return Json::parse(cache);
            } catch (const Exception& e) {
                return std::nullopt;
            }
        }
    }

    KOALABOX_API(void) init_cache(const Path& path) {
        cache_path = path;

        LOG_DEBUG("{} -> Setting cache path to: {}", __func__, path.string())
    }

    KOALABOX_API(std::optional<Json>) read_from_cache(const String& key) {
        try {
            const auto cache = read_cache().value();

            return cache.at(key);
        } catch (const Exception& e) {
            LOG_WARN("{} -> Failed to read cache from disk: {}", __func__, e.what())

            return std::nullopt;
        }
    }

    KOALABOX_API(bool) save_to_cache(const String& key, const Json& value) {
        try {
            Json new_cache;

            const auto cache = read_cache();
            if (cache) {
                new_cache = *cache;
            }

            new_cache[key] = value;

            io::write_file(cache_path, new_cache.dump(2));

            return true;
        } catch (const Exception& e) {
            LOG_ERROR("{} -> Failed to write cache to disk: {}", __func__, e.what())

            return false;
        }
    }

}
