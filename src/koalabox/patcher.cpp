#include <koalabox/patcher.hpp>

#include <chrono>
#include <cstring>
#include <regex>

namespace koalabox::patcher {

    struct PatternMask {
        [[maybe_unused]] String binary_pattern;
        [[maybe_unused]] String mask;
    };

    /**
     * Converts user-friendly hex pattern string into a byte array
     * and generates corresponding string mask
     */
    PatternMask get_pattern_and_mask(String pattern) {
        // Remove whitespaces
        pattern = std::regex_replace(pattern, std::regex("\\s+"), "");

        // Convert hex to binary
        std::stringstream patternStream;
        std::stringstream maskStream;
        for (size_t i = 0; i < pattern.length(); i += 2) {
            const std::string byteString = pattern.substr(i, 2);

            maskStream << (byteString == "??" ? '?' : 'x');

            // Handle wildcards ourselves, rest goes to strtol
            patternStream << (
                byteString == "??" ? '?' : (char) strtol(byteString.c_str(), nullptr, 16)
            );
        }

        return {patternStream.str(), maskStream.str()};
    }

    // Credit: superdoc1234
    // Source: https://www.unknowncheats.me/forum/1364641-post150.html
    char* find(char* base_address, size_t mem_length, const char* pattern, const char* mask) {
        auto DataCompare = [](
            const char* data,
            const char* mask,
            const char* ch_mask,
            char ch_last,
            size_t i_end
        ) -> bool {
            if (data[i_end] != ch_last) {
                return false;
            }

            for (size_t i = 0; i <= i_end; ++i) {
                if (ch_mask[i] == 'x' && data[i] != mask[i]) {
                    return false;
                }
            }

            return true;
        };

        auto iEnd = strlen(mask) - 1;
        auto chLast = pattern[iEnd];

        for (size_t i = 0; i < mem_length - strlen(mask); ++i) {
            if (DataCompare(base_address + i, pattern, mask, chLast, iEnd)) {
                return base_address + i;
            }
        }

        return nullptr;
    }

    // Credit: Rake
    // Source: https://guidedhacking.com/threads/external-internal-pattern-scanning-guide.14112/
    char* scan_internal(char* pMemory, size_t length, String pattern) {
        auto* const terminal_address = pMemory + length;

        char* match = nullptr;
        MEMORY_BASIC_INFORMATION mbi{};

        auto [binaryPattern, mask] = get_pattern_and_mask(std::move(pattern));

        auto* current_region = pMemory;
        do {
            // Skip irrelevant code regions
            auto query_success = VirtualQuery((LPCVOID) current_region, &mbi, sizeof(mbi));
            if (query_success && mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS) {
                TRACE(
                    "current_region: {}, mbi.BaseAddress: {}, mbi.RegionSize: {}",
                    fmt::ptr(current_region), mbi.BaseAddress, (void*) mbi.RegionSize
                )
                const auto max_address = (size_t) min(current_region + mbi.RegionSize, terminal_address);
                const auto mem_length = max_address - (size_t) current_region;
                match = find(current_region, mem_length, binaryPattern.c_str(), mask.c_str());

                if (match) {
                    break;
                }
            }

            current_region += mbi.RegionSize;
        } while (current_region < pMemory + length);

        return match;
    }

    void* find_pattern_address(
        const uint8_t* base_address,
        size_t scan_size,
        const String& name,
        const String& pattern,
        bool log_results
    ) {
        const auto t1 = std::chrono::high_resolution_clock::now();
        auto* address = scan_internal((char*) base_address, scan_size, pattern);
        const auto t2 = std::chrono::high_resolution_clock::now();

        const double elapsedTime = std::chrono::duration<double, std::milli>(t2 - t1).count();

        if (log_results) {
            if (address == nullptr) {
                logger->error("Failed to find address of '{}'. Search time: {:.2f} ms", name, elapsedTime);
            } else {
                logger->debug("'{}' address: {}. Search time: {:.2f} ms", name, fmt::ptr(address), elapsedTime);
            }
        }

        return address;
    }

    void* find_pattern_address(const MODULEINFO& process_info, const String& name, const String& pattern) {
        return find_pattern_address(
            reinterpret_cast<uint8_t*>(process_info.lpBaseOfDll),
            process_info.SizeOfImage,
            name,
            pattern,
            true
        );
    }

}
