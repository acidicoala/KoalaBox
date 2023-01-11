#pragma once

#include <koalabox/core.hpp>

namespace koalabox::ipc {

    struct Request {
        String name;
        Json::object_t args;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Request, name, args);
    };

    struct Response {
        bool success = false;
        Json::object_t data;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Response, success, data);
    };

    /// Note: This function is intended to be called from a new thread, in a try-catch block.
    KOALABOX_API(void) init_pipe_server(
        const String& pipe_id,
        const Function<Response(const Request& request)>& callback
    );

}
