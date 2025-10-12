#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <utility>

#define DLL_MAIN(...) \
    EXTERN_C [[maybe_unused]] BOOLEAN WINAPI DllMain(__VA_ARGS__)

DLL_MAIN(void* handle, uint32_t reason, void* reserved);

// TODO: Replace *_or_throw functions with a utility function
namespace koalabox::win {
    namespace fs = std::filesystem;

    std::optional<std::vector<uint8_t>> get_module_version_info(HMODULE module_handle) noexcept;
    std::vector<uint8_t> get_module_version_info_or_throw(HMODULE module_handle);
    std::optional<std::string> get_version_info_string(
        HMODULE module_handle,
        const std::string& key,
        const std::string& codepage = "040904E4" // 0x409 (English - United States) + 1252 (Windows Latin-1)
    ) noexcept;

    std::string format_message(DWORD message_id);

    std::string get_last_error();

    std::string get_module_manifest(void* module_handle);

    std::string get_module_version_or_throw(const HMODULE& module_handle);

    fs::path get_system_directory_or_throw();

    fs::path get_system_directory();

    void register_application_restart();

    std::optional<MEMORY_BASIC_INFORMATION> virtual_query(const void* pointer);

    SIZE_T write_process_memory_or_throw(
        const HANDLE& process,
        LPVOID address,
        LPCVOID buffer,
        SIZE_T size
    );

    SIZE_T write_process_memory(const HANDLE& process, LPVOID address, LPCVOID buffer, SIZE_T size);

    // Such checks are possible only on Windows, since Linux libraries don't include such metadata
    void check_self_duplicates() noexcept;

    namespace details {
        // Compile-time argument size calculator
        template <typename Ret, typename... Args>
        constexpr std::size_t function_arg_size(Ret(*)(Args...)) {
            return (0 + ... + sizeof(Args));
        }

        // Compile-time int-to-string
        template <std::size_t N>
        constexpr auto int_to_str() {
            char buf[20] = {};
            std::size_t len = 0;
            std::size_t n = N;
            do {
                buf[19 - len++] = '0' + (n % 10);
                n /= 10;
            } while (n);
            std::array<char, 20> arr = {};
            for (std::size_t i = 0; i < len; ++i) {
                arr[i] = buf[20 - len + i];
            }
            return std::pair{arr, len};
        }

        // Compile-time string length
        constexpr std::size_t cstrlen(const char* s) {
            std::size_t len = 0;
            while (s[len] != '\0') ++len;
            return len;
        }

        // Compile-time string concat
        template<std::size_t L1, std::size_t L2>
        constexpr auto concat(const char (&a)[L1], const char (&b)[L2]) {
            std::array<char, L1 + L2 - 1> result = {};
            for (std::size_t i = 0; i < L1 - 1; ++i) result[i] = a[i];
            for (std::size_t i = 0; i < L2; ++i) result[L1 - 1 + i] = b[i];
            return result;
        }
    }

    // Compose the mangled name
    template <typename Func>
    constexpr auto msvc_stdcall_mangle(Func f, const char* func_name) {
        constexpr std::size_t arg_size = function_arg_size(f);
        constexpr auto [num_str, num_len] = details::int_to_str<arg_size>();

        constexpr std::size_t name_len = details::cstrlen(func_name);

        // 1 for '_', name_len, 1 for '@', num_len, 1 for '\0'
        std::array<char, 1 + name_len + 1 + num_len + 1> out = {};
        std::size_t idx = 0;
        out[idx++] = '_';
        for (std::size_t i = 0; i < name_len; ++i) out[idx++] = func_name[i];
        out[idx++] = '@';
        for (std::size_t i = 0; i < num_len; ++i) out[idx++] = num_str[i];
        out[idx] = '\0';
        return out;
    }

}
