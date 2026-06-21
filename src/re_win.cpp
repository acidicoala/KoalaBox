#include "koalabox/re.hpp"

#ifdef _WIN64

// RtlLookupFunctionEntry / RUNTIME_FUNCTION come from <Windows.h>, force-included via the PCH. They
// exist only on 64-bit Windows: table-based exception handling (.pdata / unwind info) is a 64-bit
// ABI mechanism. 32-bit x86 unwinds through FS:[0] SEH chains and has no such table, so the resolver
// below is compiled only for _WIN64; the 32-bit definition at the bottom of the file throws.

namespace {
    // Head of the x64 UNWIND_INFO structure. The Windows SDK does not expose it publicly; we only
    // need the flags (to detect a chained fragment) and the unwind-code count (to locate the chained
    // RUNTIME_FUNCTION that follows the codes).
    struct unwind_info_head_t {
        uint8_t version_and_flags; // Version : 3 (low bits), Flags : 5 (high bits)
        uint8_t size_of_prolog;
        uint8_t count_of_codes;
        uint8_t frame_register_and_offset;
    };

    // UNW_FLAG_CHAININFO, within the 5-bit Flags field of UNWIND_INFO.
    constexpr uint8_t unwind_flag_chaininfo = 0x4;
}

namespace koalabox::re {
    // Resolves the containing function's start from the module's .pdata exception directory via
    // RtlLookupFunctionEntry. This is exact and independent of inter-function padding, unlike a
    // backward scan for int3 (0xCC) padding followed by a prologue, which fails when a function
    // starts on an alignment boundary (e.g. right after a noreturn call).
    std::optional<uintptr_t> get_function_start(const uintptr_t address) {
        DWORD64 image_base = 0;
        auto* function_entry = RtlLookupFunctionEntry(address, &image_base, nullptr);
        if(function_entry == nullptr) {
            return std::nullopt;
        }

        // A large function can be split into several .pdata fragments. Each fragment after the first
        // carries UNW_FLAG_CHAININFO and, right after its unwind codes, a RUNTIME_FUNCTION pointing
        // toward the primary fragment. RtlLookupFunctionEntry returns whichever fragment contains
        // `address`, so for an address in a secondary fragment its BeginAddress is the fragment start,
        // not the function start. Follow the chain to the root to return the actual function entry.
        // The hop count is bounded purely as defence against a malformed module.
        for(int hop = 0; hop < 64; ++hop) {
            const auto* const unwind_info = reinterpret_cast<const unwind_info_head_t*>(
                image_base + function_entry->UnwindInfoAddress
            );

            if(!((unwind_info->version_and_flags >> 3) & unwind_flag_chaininfo)) {
                break; // primary fragment reached
            }

            // The chained RUNTIME_FUNCTION follows the unwind-code array, whose entry count is rounded
            // up to even for alignment (each code is 2 bytes).
            const auto code_count = (unwind_info->count_of_codes + 1) & ~1;
            function_entry = reinterpret_cast<RUNTIME_FUNCTION*>(
                image_base + function_entry->UnwindInfoAddress
                + sizeof(unwind_info_head_t) + code_count * sizeof(uint16_t)
            );
        }

        return static_cast<uintptr_t>(image_base) + function_entry->BeginAddress;
    }
}

#else

#include "koalabox/core.hpp" // KB_RT_ERROR

namespace koalabox::re {
    // 32-bit x86 Windows has no table-based exception handling (no .pdata / RUNTIME_FUNCTION), so this
    // resolver cannot run here. Its only caller is 64-bit-only, so reaching this on x86 means the code
    // was built or dispatched for the wrong target. Returning nullopt would masquerade as a
    // legitimate "no unwind entry" result and be silently swallowed by callers, so fail loudly.
    std::optional<uintptr_t> get_function_start(uintptr_t /*address*/) {
        throw KB_RT_ERROR("koalabox::re::get_function_start is unsupported on 32-bit Windows");
    }
}

#endif
