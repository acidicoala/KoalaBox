#pragma once

#include <koalabox/core.hpp>

namespace koalabox::crypto {

    Vector<uint8_t> decode_hex_string(const String& hex_str);

    String calculate_md5(const Path& file_path);

}
