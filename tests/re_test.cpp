#include <cstdint>

#include <catch2/catch_test_macros.hpp>

#include "koalabox/re.hpp"

// Portable "don't inline this" so the address we take below is a real, distinct function entry.
#if defined(_MSC_VER)
#define KB_TEST_NOINLINE __declspec(noinline)
#else
#define KB_TEST_NOINLINE [[gnu::noinline]]
#endif

namespace {
    // Two functions with real, non-trivial bodies. The volatile accumulator and loop keep the
    // optimizer from shrinking them to nothing or folding the two together (identical-code
    // folding), which would make their entry addresses coincide and invalidate the adjacency test.
    KB_TEST_NOINLINE int sample_function_a(const int seed) {
        volatile int acc = seed;
        for(int i = 0; i < 7; ++i) {
            acc = acc * 3 + i;
        }
        return acc;
    }

    KB_TEST_NOINLINE int sample_function_b(const int seed) {
        volatile int acc = seed ^ 0x55;
        for(int i = 0; i < 11; ++i) {
            acc = acc + i * i - 1;
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
