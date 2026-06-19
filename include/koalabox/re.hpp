#pragma once

#include <cstdint>
#include <optional>

/// Reverse-engineering helpers: lower-level static/dynamic code-analysis primitives that go
/// beyond the high-level module wrappers in koalabox::lib.
namespace koalabox::re {
    /**
     * Returns the start address of the function that contains @p address, using the module's
     * authoritative unwind metadata: the `.eh_frame_hdr` binary-search table (PT_GNU_EH_FRAME)
     * on Linux, and the `.pdata` exception directory (RtlLookupFunctionEntry) on Windows.
     *
     * Unlike heuristics that scan backward for int3 (0xCC) padding followed by a prologue, this
     * is exact and does not depend on inter-function padding — which compilers omit when a
     * function happens to start on an alignment boundary (e.g. right after a noreturn call).
     *
     * Returns nullopt when @p address is not inside an unwind-described function: the address is
     * not in any loaded module's code, the module lacks a usable unwind table, or (on Windows)
     * the function is a leaf with no `.pdata` entry.
     */
    std::optional<uintptr_t> get_function_start(uintptr_t address);

    uintptr_t get_function_start_or_throw(uintptr_t address);
}
