#include <algorithm>
#include <iomanip>
#include <ios>
#include <sstream>

#include "koalabox/str.hpp"

namespace koalabox::str {
    std::string trim(std::string s) {
        // Trim leading spaces
        s.erase(
            s.begin(),
            std::ranges::find_if(s, [](const char ch) { return !std::isspace(ch); })
        );

        // Trim trailing spaces
        s.erase(
            std::find_if(
                s.rbegin(),
                s.rend(),
                [](const char ch) { return !std::isspace(ch); }
            ).base(),
            s.end()
        );

        return s;
    }

    bool eq(const std::string& s1, const std::string& s2) {
        return to_lower(s1) == to_lower(s2);
    }

    std::string to_lower(const std::string& str) {
        std::string result;
        result.resize(str.size());

        std::ranges::transform(
            str,
            result.begin(),
            //
            [](const unsigned char c) {
                return std::tolower(c);
            }
        );

        return result;
    }

    std::string to_hex(const std::string& str) {
        std::ostringstream oss;
        for(size_t i = 0; i < str.size(); ++i) {
            oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(static_cast<unsigned char>(str[i]));

            if(i != str.size() - 1) {
                oss << ' ';
            }
        }

        return oss.str();
    }

    std::string from_little_endian(const uint32_t number) {
        std::ostringstream oss;
        for(int i = 0; i < 4; ++i) {
            const uint8_t byte = (number >> (i * 8)) & 0xFF;
            if(i > 0) {
                oss << ' ';
            }
            oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(byte);
        }
        return oss.str();
    }
}
