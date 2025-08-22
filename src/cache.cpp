#include "koalabox/cache.hpp"
#include "koalabox/io.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/paths.hpp"

namespace koalabox::cache {
    namespace fs = std::filesystem;

    namespace {
        // TODO: Keep cache in memory instead and flush it on write

        nlohmann::json read_cache() {
            return nlohmann::json::parse(io::read_file(paths::get_cache_path()));
        }
    }

    nlohmann::json get(const std::string& key, const nlohmann::json& fallback) {
        const auto cache = read_cache();

        LOG_DEBUG("Cache key: \"{}\". Value: \n{}", key, cache.dump(2));

        if(cache.contains(key)) {
            return cache.at(key);
        }

        return fallback;
    }

    bool put(const std::string& key, const nlohmann::json& value) noexcept {
        try {
            static std::mutex section;
            const std::lock_guard lock(section);

            nlohmann::json new_cache;

            if(fs::exists(paths::get_cache_path())) {
                try {
                    new_cache = read_cache();
                } catch(const std::exception& e) {
                    LOG_WARN("Failed to read cache from disk: {}", e.what());
                }
            }

            new_cache[key] = value;

            io::write_file(paths::get_cache_path(), new_cache.dump(2));

            return true;
        } catch(const std::exception& e) {
            LOG_ERROR("Failed to write cache to disk: {}", e.what());

            return false;
        }
    }
}