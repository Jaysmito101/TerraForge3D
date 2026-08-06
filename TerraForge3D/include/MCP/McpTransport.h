#pragma once

#include <functional>
#include <string>

class ActionRegistry;
class MainThreadRequestQueue;
class ResourceRegistry;

namespace mcp
{
    class server;
}

namespace tf3d::mcp_layer
{

    using McpCommandRecorder = std::function<void(
        const std::string &method,
        const std::string &paramsJson,
        const std::string &sessionId)>;

    void RegisterMcpTransport(
        ::mcp::server &server,
        ActionRegistry &actions,
        ResourceRegistry &resources,
        MainThreadRequestQueue &requestQueue,
        McpCommandRecorder recordCommand);

} // namespace tf3d::mcp_layer
