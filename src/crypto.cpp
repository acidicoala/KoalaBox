#include "koalabox/crypto.hpp"
#include "koalabox/logger.hpp"

namespace koalabox::crypto {
    std::vector<uint8_t> decode_hex_string(const std::string& hex_str) {
        if(hex_str.length() < 2) {
            return {};
        }

        std::vector<uint8_t> buffer(hex_str.size() / 2);

        std::stringstream ss;
        ss << std::hex << hex_str;

        for(size_t i = 0; i < hex_str.length(); i++) {
            ss >> buffer[i];
        }

        return buffer;
    }
}
