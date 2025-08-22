#include <chrono>
#include <cstring>
#include <regex>

#include "koalabox/logger.hpp"
#include "koalabox/patcher.hpp"
#include "koalabox/win_util.hpp"

namespace koalabox::patcher {

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
        for (size_t i = 0; i < pattern.length(); i += 2) {
            const std::string byte_string = pattern.substr(i, 2);

            mask_stream << (byte_string == "??" ? '?' : 'x');

            // Handle wildcards ourselves, rest goes to strtol
            pattern_stream
                << (byte_string == "??" ? '?' : (char)strtol(byte_string.c_str(), nullptr, 16));
        }

        return {pattern_stream.str(), mask_stream.str()};
    }

    // Credit: superdoc1234
    // Source: https://www.unknowncheats.me/forum/1364641-post150.html
    uintptr_t
    find(const uintptr_t base_address, size_t mem_length, const char* pattern, const char* mask) {

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
    uintptr_t scan_internal(uintptr_t ptr_memory, size_t length, std::string pattern) {
        const uintptr_t terminal_address = ptr_memory + length;

        uintptr_t match = 0;
        MEMORY_BASIC_INFORMATION mbi{};

        auto [binaryPattern, mask] = get_pattern_and_mask(std::move(pattern));

        auto current_region = ptr_memory;
        do {
            // Skip irrelevant code regions
            auto query_success = VirtualQuery((LPCVOID)current_region, &mbi, sizeof(mbi));
            if (query_success && mbi.State == MEM_COMMIT && mbi.Protect != PAGE_NOACCESS) {
                LOG_TRACE(
                    "current_region: {}, mbi.BaseAddress: {}, mbi.RegionSize: {}",
                    (void*)current_region, mbi.BaseAddress, (void*)mbi.RegionSize
                );

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

    uintptr_t find_pattern_address(
        const uintptr_t base_address, size_t scan_size, const std::string& name,
        const std::string& pattern
    ) {
        const auto t1 = std::chrono::high_resolution_clock::now();
        const auto address = scan_internal(base_address, scan_size, pattern);
        const auto t2 = std::chrono::high_resolution_clock::now();

        const double elapsed_time = std::chrono::duration<double, std::milli>(t2 - t1).count();

        if (address) {
            LOG_DEBUG(
                "'{}' address: {}. Search time: {:.2f} ms", name, (void*)address, elapsed_time
            );
        } else {
            LOG_ERROR("Failed to find address of '{}'. Search time: {:.2f} ms", name, elapsed_time);
        }

        return address;
    }

    uintptr_t find_pattern_address(
        const MODULEINFO& process_info, const std::string& name, const std::string& pattern
    ) {
        return find_pattern_address(
            reinterpret_cast<uintptr_t>(process_info.lpBaseOfDll), process_info.SizeOfImage, name,
            pattern
        );
    }

    uintptr_t find_pattern_address(
        const HMODULE module_handle, const std::string& section_name, const std::string& name,
        const std::string& pattern
    ) {
        const auto* section = win_util::get_pe_section_or_throw(module_handle, section_name);

        const auto* section_address =
            reinterpret_cast<uint8_t*>(module_handle) + section->PointerToRawData;

        // First find the string in the .rdata section
        return find_pattern_address(
            reinterpret_cast<uintptr_t>(section_address), section->SizeOfRawData, name, pattern
        );
    }

}
