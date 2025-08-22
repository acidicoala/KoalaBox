#pragma once

#include <nlohmann/json.hpp>

namespace koalabox::ipc {
    struct Request {
        std::string name;
        nlohmann::json::object_t args;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Request, name, args);
    };

    struct Response {
        bool success = false;
        nlohmann::json::object_t data;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Response, success, data);
    };

    /// Note: This function is intended to be called from a new thread, in a try-catch block.
    void init_pipe_server(
        const std::string& pipe_id,
        const std::function<Response(const Request& request)>& callback
    );
}