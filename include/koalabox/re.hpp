#pragma once

#include <cstdint>
#include <optional>

/// Reverse-engineering helpers: lower-level static/dynamic code-analysis primitives that go
/// beyond the high-level module wrappers in koalabox::lib.
namespace koalabox::re {
    /// Start address of the function containing @p address, or nullopt if no containing function
    /// can be resolved.
    std::optional<uintptr_t> get_function_start(uintptr_t address);

    uintptr_t get_function_start_or_throw(uintptr_t address);
}
