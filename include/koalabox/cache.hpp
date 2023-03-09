#pragma once

#include <koalabox/core.hpp>

/**
 * This namespace contains utility functions for reading from and writing to cache file on disk.
 * All functions are intended to be safe to call, i.e. they should not throw exceptions.
 */
namespace koalabox::cache {

    KOALABOX_API(Json) get(const String& key, const Json& value = Json());

    KOALABOX_API(bool) put(const String& key, const Json& value) noexcept;

}
