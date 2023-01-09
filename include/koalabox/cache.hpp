#pragma once

#include <koalabox/core.hpp>

/**
 * This namespace contains utility functions for reading from and writing to cache file on disk.
 * All functions are intended to be safe to call, i.e. they should not throw exceptions.
 */
namespace koalabox::cache {

    KOALABOX_API(void) init_cache(const Path& path) noexcept;

    KOALABOX_API(Json) read_from_cache(const String& key);

    KOALABOX_API(bool) save_to_cache(const String& key, const Json& value) noexcept;

}
