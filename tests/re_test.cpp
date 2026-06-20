#include <cstdint>

#if defined(_WIN32)
#include <windows.h> // GetModuleHandleW, RUNTIME_FUNCTION, IMAGE_* (function-table walk below)
#endif

#include <catch2/catch_test_macros.hpp>

#include "koalabox/re.hpp"

// Portable "don't inline this" so the address we take below is a real, distinct function entry.
#if defined(_MSC_VER)
#define KB_TEST_NOINLINE __declspec(noinline)
#else
#define KB_TEST_NOINLINE [[gnu::noinline]]
#endif

namespace {
    // A sink the optimizer cannot see through (a separate noinline function whose only observable
    // effect is a volatile write), so calls to it below cannot be elided. This matters on Windows:
    // get_function_start relies on RtlLookupFunctionEntry, which returns nullopt for a *leaf*
    // function that has no .pdata/unwind entry. A function whose body is merely a volatile local
    // plus arithmetic can compile to exactly such a leaf in optimized builds. Forcing a real call
    // makes each sample below non-leaf, so the compiler must emit a stack frame and a .pdata entry —
    // matching the non-leaf functions the analyzer actually resolves in steamclient.
    KB_TEST_NOINLINE int sink(const int value) {
        volatile int v = value;
        return v;
    }

    // Two functions with real, non-trivial bodies. The differing constants/iteration counts keep
    // the optimizer from folding the two together (identical-code folding), which would make their
    // entry addresses coincide and invalidate the adjacency test; the sink() calls keep them
    // non-leaf so they have the .pdata entry get_function_start needs on Windows (see sink above).
    KB_TEST_NOINLINE int sample_function_a(const int seed) {
        int acc = seed;
        for(int i = 0; i < 7; ++i) {
            acc = sink(acc * 3 + i);
        }
        return acc;
    }

    KB_TEST_NOINLINE int sample_function_b(const int seed) {
        int acc = seed ^ 0x55;
        for(int i = 0; i < 11; ++i) {
            acc = sink(acc + i * i - 1);
        }
        return acc;
    }

    uintptr_t address_of(int (*fn)(int)) {
        return reinterpret_cast<uintptr_t>(fn);
    }
}

TEST_CASE("get_function_start resolves a function's own entry", "[re]") {
    const auto entry = address_of(&sample_function_a);

    const auto result = koalabox::re::get_function_start(entry);

    REQUIRE(result.has_value());
    REQUIRE(*result == entry);
}

TEST_CASE("get_function_start resolves an address inside a function to its entry", "[re]") {
    const auto entry = address_of(&sample_function_a);

    // A few bytes past the prologue still belongs to the same function. The unwind table maps it
    // back to the entry, which is exactly what the int3-padding heuristic cannot do reliably.
    const auto result = koalabox::re::get_function_start(entry + 4);

    REQUIRE(result.has_value());
    REQUIRE(*result == entry);
}

TEST_CASE("get_function_start distinguishes adjacent functions", "[re]") {
    const auto entry_a = address_of(&sample_function_a);
    const auto entry_b = address_of(&sample_function_b);

    REQUIRE(entry_a != entry_b);
    REQUIRE(koalabox::re::get_function_start(entry_a) == entry_a);
    REQUIRE(koalabox::re::get_function_start(entry_b) == entry_b);
}

TEST_CASE("get_function_start returns nullopt for an address outside any module", "[re]") {
    // A low address that cannot fall inside any loaded module's mapped code.
    const auto result = koalabox::re::get_function_start(0x1000);

    REQUIRE_FALSE(result.has_value());
}

#if defined(_WIN32)
namespace {
    // Finds the first .pdata RUNTIME_FUNCTION in this module that is a *chained* fragment
    // (UNW_FLAG_CHAININFO): a piece of a function the optimizer split off, whose own BeginAddress is
    // not the function's start. Returns its runtime begin address, or 0 if none exist. The test
    // binary links enough optimized code that thousands do.
    uintptr_t first_chained_fragment() {
        const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        if(!base) {
            return 0;
        }

        const auto* const dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto* const nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
        if(dir.VirtualAddress == 0 || dir.Size == 0) {
            return 0;
        }

        const auto* const functions = reinterpret_cast<const RUNTIME_FUNCTION*>(base + dir.VirtualAddress);
        const auto count = dir.Size / sizeof(RUNTIME_FUNCTION);

        constexpr uint8_t unw_flag_chaininfo = 0x4; // within the 5-bit Flags field of UNWIND_INFO
        for(size_t i = 0; i < count; ++i) {
            const auto* const unwind_info =
                reinterpret_cast<const uint8_t*>(base + functions[i].UnwindInfoAddress);
            if((unwind_info[0] >> 3) & unw_flag_chaininfo) {
                return base + functions[i].BeginAddress;
            }
        }

        return 0;
    }
}

// Regression test for the chained-fragment bug: a large function split across several .pdata entries.
// RtlLookupFunctionEntry returns whichever fragment contains the address, so for a secondary fragment
// its BeginAddress is the fragment start, not the function's - get_function_start must follow
// UNW_FLAG_CHAININFO back to the primary. The single-fragment sample functions above never exercised
// this, which is why the bug shipped.
TEST_CASE("get_function_start resolves a chained .pdata fragment to the primary function", "[re]") {
    const auto fragment = first_chained_fragment();
    REQUIRE(fragment != 0); // the test binary contains many chained functions

    const auto start = koalabox::re::get_function_start(fragment);
    REQUIRE(start.has_value());

    // The pre-fix version returned RtlLookupFunctionEntry's raw answer - the fragment's own
    // BeginAddress (i.e. == fragment). Following the chain to the primary yields a different address.
    CHECK(*start != fragment);

    // The resolved primary is itself a real, non-chained entry, so resolving it again is a no-op.
    CHECK(koalabox::re::get_function_start(*start) == *start);
}
#endif // _WIN32
