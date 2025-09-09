#include <fstream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "koalabox/io.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/path.hpp"

namespace koalabox::io {
    std::string read_file(const fs::path& file_path) {
        std::ifstream input_stream(file_path);
        input_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        return {std::istreambuf_iterator{input_stream}, {}};
    }

    bool write_file(const fs::path& file_path, const std::string& contents) noexcept {
        try {
            if(!std::filesystem::exists(file_path)) {
                std::filesystem::create_directories(file_path.parent_path());
            }

            // Use binary mode to prevent windows from writing \n as CRLF
            std::ofstream output_stream(file_path, std::ios::binary);
            if(output_stream.good()) {
                output_stream << contents;

                LOG_TRACE(R"(Writing file to disk: "{}")", path::to_str(file_path));
                return true;
            }

            LOG_ERROR(
                R"(Error opening output stream: "{}". Flags: {})",
                path::to_str(file_path),
                output_stream.flags()
            );
            return false;
        } catch(const std::exception& e) {
            LOG_ERROR("Unexpected exception caught: {}", e.what());
            return false;
        }
    }

    // Source: https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect#example-code
    bool is_local_port_in_use(const int port) {
        //----------------------
        // Initialize Winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if(iResult != NO_ERROR) {
            // LOG_ERROR("WSAStartup error: {}", iResult)
            return false;
        }

        //----------------------
        // Create a SOCKET for connecting to server
        SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(ConnectSocket == INVALID_SOCKET) {
            // LOG_ERROR("socket(...) error: {}", WSAGetLastError())
            WSACleanup();
            return false;
        }

        //----------------------
        // The sockaddr_in structure specifies the address family,
        // IP address, and port of the server to be connected to.
        sockaddr_in client_service{};
        client_service.sin_family = AF_INET;
        client_service.sin_port = htons(port);
        iResult = InetPton(AF_INET, L"127.0.0.1", &client_service.sin_addr.s_addr);
        if(iResult != 1) {
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
        iResult = connect(
            ConnectSocket,
            reinterpret_cast<SOCKADDR*>(&client_service),
            sizeof(client_service)
        );
        if(iResult == SOCKET_ERROR) {
            // LOG_ERROR("connect(...) error: {}", WSAGetLastError())
            close_socket();
            return false;
        }

        // If program has reached here, it means that the port was open
        close_socket();

        return true;
    }
}