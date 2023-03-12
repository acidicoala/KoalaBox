#include <koalabox/io.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/util.hpp>

#include <miniz.h>

#include <fstream>
#include <WinSock2.h>
#include <WS2tcpip.h>

namespace koalabox::io {

    String read_file(const Path& file_path) {
        std::ifstream input_stream(file_path);
        input_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            return { std::istreambuf_iterator<char>{ input_stream }, {}};
        } catch (const std::system_error& e) {
            const auto& code = e.code();
            throw std::runtime_error(
                fmt::format("Input file stream error code: {}, message: {}", code.value(),
                    code.message())
            );
        }
    }

    bool write_file(const Path& file_path, const String& contents) noexcept {
        try {
            if (!std::filesystem::exists(file_path)) {
                std::filesystem::create_directories(file_path.parent_path());
            }

            std::ofstream output_stream(file_path);
            if (output_stream.good()) {
                output_stream << contents;

                LOG_DEBUG(R"(Writing file to disk: "{}")", file_path.string())
                return true;
            }

            LOG_ERROR(
                R"(Error opening output stream: "{}". Flags: {})",
                file_path.string(), output_stream.flags()
            )
            return false;
        } catch (const Exception& e) {
            LOG_ERROR("Unexpected exception caught: {}", e.what())
            return false;
        }
    }

    bool unzip_file(
        const Path& source_zip,
        const String& target_file,
        const Path& destination_dir
    ) noexcept {
        try {
            // open the zip archive for reading
            DECLARE_STRUCT(mz_zip_archive, archive);
            if (!mz_zip_reader_init_file(&archive, source_zip.string().c_str(), 0)) {
                throw KException("Error opening archive");
            }

            // get the index of the file in the archive
            mz_uint file_index = mz_zip_reader_locate_file(
                &archive,
                target_file.c_str(),
                nullptr,
                0
            );

            if (file_index == -1) {
                mz_zip_reader_end(&archive);
                throw KException("File not found in archive");
            }

            // get the uncompressed size of the file
            mz_zip_archive_file_stat file_stat;
            if (!mz_zip_reader_file_stat(&archive, file_index, &file_stat)) {
                mz_zip_reader_end(&archive);
                throw KException("Error getting file stats");
            }

            // allocate a buffer to hold the uncompressed file data
            void* file_data = malloc(file_stat.m_uncomp_size);
            if (!file_data) {
                // error allocating buffer
                mz_zip_reader_end(&archive);
                throw KException("Error allocating buffer");
            }

            // read the file data into the buffer
            if (!mz_zip_reader_extract_to_mem(
                &archive,
                file_index,
                file_data,
                file_stat.m_uncomp_size,
                0
            )) {
                free(file_data);
                mz_zip_reader_end(&archive);
                throw KException("Error extracting file");
            }

            // write the file data to disk
            const auto destination_path = destination_dir / Path(target_file).filename();
            FILE* fp = fopen(destination_path.string().c_str(), "wb");
            if (!fp) {
                // error creating file
                free(file_data);
                mz_zip_reader_end(&archive);
                throw KException("Error creating file");
            }
            fwrite(file_data, 1, file_stat.m_uncomp_size, fp);
            fclose(fp);

            // clean up
            free(file_data);
            mz_zip_reader_end(&archive);

            return true;
        } catch (const Exception& e) {
            LOG_ERROR(
                R"(Error unzipping file "{}" from "{}" to "{}": {})",
                target_file, source_zip.string(), destination_dir.string(), e.what()
            )

            return false;
        }
    }

    // Source: https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect#example-code
    bool is_local_port_in_use(int port) {
        //----------------------
        // Initialize Winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != NO_ERROR) {
            // LOG_ERROR("WSAStartup error: {}", iResult)
            return false;
        }

        //----------------------
        // Create a SOCKET for connecting to server
        SOCKET ConnectSocket;
        ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (ConnectSocket == INVALID_SOCKET) {
            // LOG_ERROR("socket(...) error: {}", WSAGetLastError())
            WSACleanup();
            return false;
        }

        //----------------------
        // The sockaddr_in structure specifies the address family,
        // IP address, and port of the server to be connected to.
        DECLARE_STRUCT(sockaddr_in, client_service);
        client_service.sin_family = AF_INET;
        client_service.sin_port = htons(port);
        iResult = InetPton(AF_INET, L"127.0.0.1", &client_service.sin_addr.s_addr);
        if (iResult != 1) {
            // LOG_ERROR("InetPton Error: {}", WSAGetLastError())
            return false;
        }

        const auto close_socket = [&]() {
            iResult = closesocket(ConnectSocket);

            /*if (iResult == SOCKET_ERROR) {
                LOG_ERROR("closesocket(...) error: {}", WSAGetLastError())
            }*/

            WSACleanup();
        };

        //----------------------
        // Connect to server.
        iResult = connect(ConnectSocket, (SOCKADDR*) &client_service, sizeof(client_service));
        if (iResult == SOCKET_ERROR) {
            // LOG_ERROR("connect(...) error: {}", WSAGetLastError())
            close_socket();
            return false;
        }

        // If program has reached here, it means that the port was open
        close_socket();

        return true;
    }
}
