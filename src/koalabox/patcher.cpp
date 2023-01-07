#include <koalabox/patcher.hpp>
#include <koalabox/logger.hpp>

#include <chrono>
#include <cstring>
#include <regex>

namespace koalabox::patcher {

    struct PatternMask {
        String binary_pattern;
        String mask;
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
    uintptr_t find(const uintptr_t base_address, size_t mem_length, const char* pattern, const char* mask) {

        auto i_end = strlen(mask) - 1;

        auto DataCompare = [&](const char* data) -> bool {
            if (data[i_end] != pattern[i_end]) {
                return false;
            }

            for (size_t i = 0; i <= i_end; ++i) {
                if (mask[i] == 'x' && data[i] != pattern[i]) {
                    return false;
                }
            }

            return true;
        };


        for (size_t i = 0; i < mem_length - strlen(mask); ++i) {
            if (DataCompare(reinterpret_cast<char*>(base_address + i))) {
                return base_address + i;
            }
        }

        return 0;
    }

    // Credit: Rake
    // Source: https://guidedhacking.com/threads/external-internal-pattern-scanning-guide.14112/
    uintptr_t scan_internal(uintptr_t ptr_memory, size_t length, String pattern) {
        const uintptr_t terminal_address = ptr_memory + length;

        uintptr_t match = 0;
        MEMORY_BASIC_INFORMATION mbi{};

        auto [binaryPattern, mask] = get_pattern_and_mask(std::move(pattern));

        auto current_region = ptr_memory;
        do {
            // Skip irrelevant code regions
            auto query_success = VirtualQuery((LPCVOID) current_region, &mbi, sizeof(mbi));
            if (query_success && mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS) {
                LOG_TRACE(
                    "{} -> current_region: {}, mbi.BaseAddress: {}, mbi.RegionSize: {}",
                    __func__, (void*) current_region, mbi.BaseAddress, (void*) mbi.RegionSize
                )

                const uintptr_t potential_end = current_region + mbi.RegionSize;
                const auto max_address = std::min(potential_end, terminal_address);
                const auto mem_length = max_address - current_region;
                match = find(current_region, mem_length, binaryPattern.c_str(), mask.c_str());

                if (match) {
                    break;
                }
            }

            current_region += mbi.RegionSize;
        } while (current_region < ptr_memory + length);

        return match;
    }

    KOALABOX_API(uintptr_t) find_pattern_address(
        const uintptr_t base_address,
        size_t scan_size,
        const String& name,
        const String& pattern
    ) {
        const auto t1 = std::chrono::high_resolution_clock::now();
        const auto address = scan_internal(base_address, scan_size, pattern);
        const auto t2 = std::chrono::high_resolution_clock::now();

        const double elapsed_time = std::chrono::duration<double, std::milli>(t2 - t1).count();

        if (address) {
            LOG_DEBUG("'{}' address: {}. Search time: {:.2f} ms", name, (void*) address, elapsed_time)
        } else {
            LOG_ERROR("Failed to find address of '{}'. Search time: {:.2f} ms", name, elapsed_time)
        }

        return address;
    }

    KOALABOX_API(uintptr_t) find_pattern_address(const MODULEINFO& process_info, const String& name,
                                                 const String& pattern) {
        return find_pattern_address(
            reinterpret_cast<uintptr_t>(process_info.lpBaseOfDll),
            process_info.SizeOfImage,
            name,
            pattern
        );
    }

}
