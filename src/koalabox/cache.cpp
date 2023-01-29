#include <koalabox/cache.hpp>
#include <koalabox/io.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/paths.hpp>

namespace koalabox::cache {

    namespace {
        Json read_cache() {
            return Json::parse(
                io::read_file(
                    koalabox::paths::get_cache_path()
                )
            );
        }
    }

    KOALABOX_API(Json) read_from_cache(const String& key) {
        const auto cache = read_cache();

        LOG_DEBUG("Cache key: {}. Value: \n{}", key, cache.dump(2))

        return cache.at(key);
    }

    KOALABOX_API(bool) save_to_cache(const String& key, const Json& value) noexcept {
        try {
            Json new_cache;
            try {
                new_cache = read_cache();
            } catch (const Exception& e) {
                LOG_WARN("Failed to read cache from disk: {}", e.what())
            }

            new_cache[key] = value;

            io::write_file(koalabox::paths::get_cache_path(), new_cache.dump(2));

            return true;
        } catch (const Exception& e) {
            LOG_ERROR("Failed to write cache to disk: {}", e.what())

            return false;
        }
    }

}
