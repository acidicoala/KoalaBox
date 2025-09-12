#include <chrono>
#include <cstring>
#include <regex>

#include "koalabox/patcher.hpp"
#include "koalabox/logger.hpp"

namespace {
    struct pattern_mask {
        std::string binary_pattern;
        std::string mask;
    };

    /**
     * Converts user-friendly hex pattern string into a byte array
     * and generates corresponding string mask
     */
    pattern_mask get_pattern_and_mask(std::string pattern) {
        // Remove whitespaces
        pattern = std::regex_replace(pattern, std::regex("\\s+"), "");

        // Convert hex to binary
        std::stringstream pattern_stream;
        std::stringstream mask_stream;
        for(size_t i = 0; i < pattern.length(); i += 2) {
            const std::string byte_string = pattern.substr(i, 2);

            mask_stream << (byte_string == "??" ? '?' : 'x');

            // Handle wildcards ourselves, rest goes to strtol
            pattern_stream
                << (byte_string == "??" ? '?' : static_cast<char>(strtol(byte_string.c_str(), nullptr, 16)));
        }

        return {pattern_stream.str(), mask_stream.str()};
    }

    // Credit: superdoc1234
    // Source: https://www.unknowncheats.me/forum/1364641-post150.html
    uintptr_t find(
        const uintptr_t base_address,
        const size_t mem_length,
        const char* pattern,
        const char* mask
    ) {
        const auto i_end = strlen(mask) - 1;

        auto DataCompare = [&](const char* data) -> bool {
            if(data[i_end] != pattern[i_end]) {
                return false;
            }

            for(size_t i = 0; i <= i_end; ++i) {
                if(mask[i] == 'x' && data[i] != pattern[i]) {
                    return false;
                }
            }

            return true;
        };

        for(size_t i = 0; i < mem_length - strlen(mask); ++i) {
            if(DataCompare(reinterpret_cast<char*>(base_address + i))) {
                return base_address + i;
            }
        }

        return 0;
    }
}

namespace koalabox::patcher {
    uintptr_t find_pattern_address(
        const uintptr_t base_address,
        const size_t scan_size,
        const std::string& name,
        const std::string& pattern
    ) {
        const auto t1 = std::chrono::high_resolution_clock::now();

        LOG_TRACE(
            "Scanning region {}-{}",
            reinterpret_cast<void*>(base_address),
            reinterpret_cast<void*>(base_address + scan_size)
        );

        auto [binaryPattern, mask] = get_pattern_and_mask(pattern);
        const auto address = find(base_address, scan_size, binaryPattern.c_str(), mask.c_str());

        const auto t2 = std::chrono::high_resolution_clock::now();
        const double elapsed_time = std::chrono::duration<double, std::milli>(t2 - t1).count();

        if(address) {
            LOG_DEBUG(
                "'{}' address: {}. Search time: {:.2f} ms",
                name,
                reinterpret_cast<void*>(address),
                elapsed_time
            );
        } else {
            LOG_ERROR("Failed to find address of '{}'. Search time: {:.2f} ms", name, elapsed_time);
        }

        return address;
    }
}
