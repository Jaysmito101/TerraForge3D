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

using McpCommandRecorder = std::function<void(
    const std::string &method,
    const std::string &paramsJson,
    const std::string &sessionId)>;

void RegisterMcpTransport(
    mcp::server &server,
    ActionRegistry &actions,
    ResourceRegistry &resources,
    MainThreadRequestQueue &requestQueue,
    McpCommandRecorder recordCommand);
