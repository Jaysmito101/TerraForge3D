#include "MCP/McpStatus.h"

#include "Data/VersionInfo.h"

#include "mcp_server.h"

namespace tf3d::mcp_layer
{

    nlohmann::json BuildMcpStatus(const ApplicationState *applicationState)
    {
        return {
            {"applicationAttached", applicationState != nullptr},
            {"version", TERR3D_VERSION_STRING},
            {"protocolVersion", ::mcp::MCP_VERSION},
            {"transport", "streamable-http"}};
    }

} // namespace tf3d::mcp_layer
