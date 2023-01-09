#include <koalabox/cache.hpp>
#include <koalabox/io.hpp>
#include <koalabox/logger.hpp>

namespace koalabox::cache {

    namespace {
        Path cache_path;

        Json read_cache() {
            const auto cache_str = io::read_file(cache_path);

            LOG_TRACE("Disk cache: \n{}", cache_str)

            return Json::parse(io::read_file(cache_path));
        }
    }

    KOALABOX_API(void) init_cache(const Path& path) noexcept {
        cache_path = path;

        LOG_DEBUG(R"(Setting cache path to: "{}")", path.string())
    }

    KOALABOX_API(Json) read_from_cache(const String& key) {
        const auto cache = read_cache();

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

            io::write_file(cache_path, new_cache.dump(2));

            return true;
        } catch (const Exception& e) {
            LOG_ERROR("Failed to write cache to disk: {}", e.what())

            return false;
        }
    }

}
