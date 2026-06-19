#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

#include <elf.h> // PT_LOAD, PT_GNU_EH_FRAME
#include <link.h> // dl_iterate_phdr, dl_phdr_info, ElfW

#include "koalabox/re.hpp"
#include "koalabox/logger.hpp"

namespace {
    // DWARF exception-handling pointer encodings, as used in the `.eh_frame_hdr` header. We
    // support only the standard x86-64 layout emitted by GCC/Clang; any other encoding is
    // rejected loudly rather than silently misparsed.
    constexpr uint8_t DW_EH_PE_udata4 = 0x03;
    constexpr uint8_t DW_EH_PE_sdata4 = 0x0b;
    constexpr uint8_t DW_EH_PE_pcrel = 0x10;
    constexpr uint8_t DW_EH_PE_datarel = 0x30;

    // A loaded module reduced to what we need to resolve function starts within it.
    struct module_unwind_t {
        uintptr_t range_lo; // module's lowest loaded address
        uintptr_t range_hi; // module's highest loaded address (exclusive)
        uintptr_t eh_frame_hdr; // runtime address of .eh_frame_hdr (0 => unsupported/absent)
        uint32_t fde_count; // number of entries in the binary-search table
    };

    // Lazily-populated, append-only cache of modules we have already resolved. The interface and
    // family-bypass scans probe the same module up to millions of times, so each module's unwind
    // table is resolved once (one dl_iterate_phdr walk) and then binary-searched in place.
    //
    // Not synchronized: steamclient analysis runs single-threaded during injection startup.
    std::vector<module_unwind_t>& module_cache() {
        static std::vector<module_unwind_t> cache;
        return cache;
    }

    // Walks loaded modules to find the one containing @p pc, recording its address span and (when
    // present and supported) its parsed .eh_frame_hdr. range_hi is left 0 if no module contains pc.
    module_unwind_t resolve_module(const uintptr_t pc) {
        struct context_t {
            uintptr_t pc;
            module_unwind_t result;
        } context{pc, {}};

        dl_iterate_phdr(
            [](dl_phdr_info* const info, size_t, void* const data) -> int {
                auto* const ctx = static_cast<context_t*>(data);
                const auto base = static_cast<uintptr_t>(info->dlpi_addr);

                auto range_lo = UINTPTR_MAX;
                uintptr_t range_hi = 0;
                uintptr_t eh_frame_hdr = 0;
                bool pc_in_module = false;

                for(int i = 0; i < info->dlpi_phnum; ++i) {
                    const auto& phdr = info->dlpi_phdr[i];

                    if(phdr.p_type == PT_LOAD) {
                        const auto segment_lo = base + phdr.p_vaddr;
                        const auto segment_hi = segment_lo + phdr.p_memsz;
                        range_lo = std::min(range_lo, segment_lo);
                        range_hi = std::max(range_hi, segment_hi);
                        if(ctx->pc >= segment_lo && ctx->pc < segment_hi) {
                            pc_in_module = true;
                        }
                    } else if(phdr.p_type == PT_GNU_EH_FRAME) {
                        eh_frame_hdr = base + phdr.p_vaddr;
                    }
                }

                if(!pc_in_module) {
                    return 0; // keep searching other modules
                }

                ctx->result.range_lo = range_lo;
                ctx->result.range_hi = range_hi;

                if(eh_frame_hdr != 0) {
                    const auto* const header = reinterpret_cast<const uint8_t*>(eh_frame_hdr);
                    const bool supported =
                        header[0] == 1 && // version
                        header[1] == (DW_EH_PE_pcrel | DW_EH_PE_sdata4) && // eh_frame_ptr_enc
                        header[2] == DW_EH_PE_udata4 && // fde_count_enc
                        header[3] == (DW_EH_PE_datarel | DW_EH_PE_sdata4); // table_enc

                    if(supported) {
                        // Header layout: 4 encoding bytes, a 4-byte eh_frame_ptr, a 4-byte
                        // fde_count, then the search table.
                        ctx->result.eh_frame_hdr = eh_frame_hdr;
                        ctx->result.fde_count = *reinterpret_cast<const uint32_t*>(eh_frame_hdr + 8);
                    } else {
                        LOG_ERROR(
                            "Unsupported .eh_frame_hdr encoding [{:#04x} {:#04x} {:#04x} {:#04x}] in module '{}'",
                            header[0], header[1], header[2], header[3], info->dlpi_name
                        );
                    }
                }

                return 1; // stop: the module containing pc has been found
            },
            &context
        );

        return context.result;
    }

    const module_unwind_t* get_cached_module(const uintptr_t pc) {
        for(const auto& module : module_cache()) {
            if(pc >= module.range_lo && pc < module.range_hi) {
                return &module;
            }
        }

        const auto resolved = resolve_module(pc);
        if(resolved.range_hi == 0) {
            return nullptr; // pc is not inside any loaded module
        }

        module_cache().push_back(resolved);
        return &module_cache().back();
    }
}

namespace koalabox::re {
    std::optional<uintptr_t> get_function_start(const uintptr_t address) {
        const auto* const module = get_cached_module(address);
        if(module == nullptr || module->eh_frame_hdr == 0 || module->fde_count == 0) {
            return std::nullopt;
        }

        // The search table holds fde_count entries of {sdata4 initial_location, sdata4 fde_address},
        // sorted ascending by initial_location, each value relative to the .eh_frame_hdr address.
        // Binary-search for the greatest initial_location <= address.
        const auto* const table = reinterpret_cast<const int32_t*>(module->eh_frame_hdr + 12);

        size_t low = 0;
        size_t high = module->fde_count;
        std::optional<uintptr_t> function_start;
        while(low < high) {
            const auto mid = low + (high - low) / 2;
            const auto candidate = module->eh_frame_hdr + static_cast<intptr_t>(table[mid * 2]);
            if(candidate <= address) {
                function_start = candidate;
                low = mid + 1;
            } else {
                high = mid;
            }
        }

        return function_start;
    }
}
