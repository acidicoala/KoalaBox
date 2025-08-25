#include <wincrypt.h>

#include "koalabox/crypto.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"
#include "koalabox/str.hpp"
#include "koalabox/win.hpp"

namespace koalabox::crypto {
    std::vector<uint8_t> decode_hex_string(const std::string& hex_str) {
        if(hex_str.length() < 2) {
            return {};
        }

        std::vector<uint8_t> buffer(hex_str.size() / 2);

        std::stringstream ss;
        ss << std::hex << hex_str;

        for(size_t i = 0; i < hex_str.length(); i++) {
            ss >> buffer[i];
        }

        return buffer;
    }

    // Source:
    // https://learn.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--creating-an-md-5-hash-from-file-content
    std::string calculate_md5(const fs::path& file_path) {
        std::string result_buffer(32, '\0');
        static constexpr auto BUFFER_SIZE = 1024U * 1024U; // 1 Mb
        static constexpr auto MD5_LEN = 16U;

        BOOL result = FALSE;
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        HANDLE hFile = nullptr;

        const auto file_path_wstr = path::to_wstr(file_path);

        hFile = CreateFile(
            file_path_wstr.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN,
            nullptr
        );

        if(INVALID_HANDLE_VALUE == hFile) {
            LOG_ERROR(
                "Error opening file {}. Error: {}",
                str::to_str(file_path_wstr),
                win::get_last_error()
            );

            return "";
        }

        // Get handle to the crypto provider
        if(!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            LOG_ERROR("CryptAcquireContext error. Error: {}", win::get_last_error());

            CloseHandle(hFile);
            return "";
        }

        if(!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
            LOG_ERROR("CryptAcquireContext error. Error: {}", win::get_last_error());

            CloseHandle(hFile);
            CryptReleaseContext(hProv, 0);
            return "";
        }

        auto* rgb_file = new BYTE[BUFFER_SIZE];
        DWORD bytes_read = 0;
        while((result = ReadFile(hFile, rgb_file, BUFFER_SIZE, &bytes_read, nullptr))) {
            if(not bytes_read) {
                break;
            }

            if(!CryptHashData(hHash, rgb_file, bytes_read, 0)) {
                LOG_ERROR("CryptHashData error. Error: {}", win::get_last_error());

                CryptReleaseContext(hProv, 0);
                CryptDestroyHash(hHash);
                CloseHandle(hFile);
                return "";
            }
        }
        delete[] rgb_file;

        if(!result) {
            LOG_ERROR("ReadFile error. Error: {}", win::get_last_error());

            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return "";
        }

        BYTE rgb_hash[MD5_LEN];
        DWORD hash_length = MD5_LEN;
        if(CryptGetHashParam(hHash, HP_HASHVAL, rgb_hash, &hash_length, 0)) {
            for(WORD i = 0; i < hash_length; i++) {
                static constexpr CHAR rgb_digits[] = "0123456789abcdef";
                result_buffer[i * 2] = rgb_digits[rgb_hash[i] >> 4];
                result_buffer[i * 2 + 1] = rgb_digits[rgb_hash[i] & 0xf];
            }
        } else {
            LOG_ERROR("ReadFile CryptGetHashParam. Error: {}", win::get_last_error());
        }

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);

        return result_buffer;
    }
}
