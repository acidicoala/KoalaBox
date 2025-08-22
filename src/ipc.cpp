#include "koalabox/ipc.hpp"
#include "koalabox/logger.hpp"
#include "koalabox/str.hpp"
#include "koalabox/util.hpp"
#include "koalabox/win_util.hpp"

namespace koalabox::ipc {
    constexpr auto BUFFER_SIZE = 32 * 1024; // 32Kb

    // Source: https://learn.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server
    void init_pipe_server(
        const std::string& pipe_id,
        const std::function<Response(const Request & request)>& callback
    ) {
        const auto pipe_name = R"(\\.\pipe\)" + pipe_id;
        const auto pipe_name_wstr = str::to_wstr(pipe_name);

        while(true) {
            LOG_DEBUG("{} -> Awaiting client connections on '{}'", __func__, pipe_name);

            const HANDLE hPipe = CreateNamedPipe(
                pipe_name_wstr.c_str(),
                // pipe name
                PIPE_ACCESS_DUPLEX,
                // read/write access
                PIPE_TYPE_MESSAGE | // message type pipe
                PIPE_READMODE_MESSAGE | // message-read mode
                PIPE_WAIT,
                // blocking mode
                PIPE_UNLIMITED_INSTANCES,
                // max. instances
                BUFFER_SIZE,
                // output buffer size
                BUFFER_SIZE,
                // input buffer size
                0,
                // client time-out
                nullptr // default security attribute
            );

            if(hPipe == INVALID_HANDLE_VALUE) {
                LOG_ERROR(
                    "{} -> Error creating a named pipe. Last error: {}",
                    __func__,
                    koalabox::win_util::get_last_error()
                );

                break;
            }

            if(const auto connected =
                    ConnectNamedPipe(hPipe, nullptr) || (GetLastError() == ERROR_PIPE_CONNECTED);
                !connected) {
                // The client could not connect, so close the pipe.
                CloseHandle(hPipe);
                break;
            }

            try {
                LOG_DEBUG("{} -> Received client connection", __func__);

                // Loop until done reading
                char request_buffer[BUFFER_SIZE] = {'\0'};
                DWORD bytes_read = 0;
                const auto read_success = ReadFile(
                    hPipe,
                    // handle to pipe
                    request_buffer,
                    // buffer to receive data
                    BUFFER_SIZE,
                    // size of buffer
                    &bytes_read,
                    // number of bytes read
                    nullptr // not overlapped I/O
                );

                if(!read_success || bytes_read == 0) {
                    if(GetLastError() == ERROR_BROKEN_PIPE) {
                        LOG_ERROR("{} -> Client disconnected", __func__);
                    } else {
                        LOG_ERROR(
                            "{} -> ReadFile error: {}",
                            __func__,
                            koalabox::win_util::get_last_error()
                        );
                    }
                    return;
                }

                Response response;
                try {
                    const auto json = nlohmann::json::parse(request_buffer);

                    LOG_DEBUG("{} -> Parsed request json: \n{}", __func__, json.dump(2));

                    const auto request = json.get<Request>();

                    response = callback(request);
                } catch(const std::exception& e) {
                    LOG_ERROR("{} -> Error processing message: {}", __func__, e.what());

                    response.success = false;
                    response.data["error_message"] = e.what();
                }

                const auto response_str = nlohmann::json(response).dump(2);

                LOG_DEBUG("{} -> Response json: \n{}", __func__, response_str);

                // Write the reply to the pipe.
                DWORD bytes_written = 0;
                const auto write_success = WriteFile(
                    hPipe,
                    // handle to pipe
                    response_str.c_str(),
                    // buffer to write from
                    response_str.size(),
                    // number of bytes to write
                    &bytes_written,
                    // number of bytes written
                    nullptr // not overlapped I/O
                );

                if(!write_success || response_str.size() != bytes_written) {
                    LOG_ERROR(
                        "{} -> Error writing file. Last error: {}",
                        __func__,
                        koalabox::win_util::get_last_error()
                    );
                    return;
                }

                // Flush the pipe to allow the client to read the pipe's contents
                // before disconnecting. Then disconnect the pipe, and close the
                // handle to this pipe instance.

                FlushFileBuffers(hPipe);
                DisconnectNamedPipe(hPipe);
                CloseHandle(hPipe);

                LOG_DEBUG("{} -> Finished processing request", __func__);
            } catch(const std::exception& e) {
                LOG_ERROR("Pipe server error processing client connection: {}", e.what());
            }
        }
    }
}