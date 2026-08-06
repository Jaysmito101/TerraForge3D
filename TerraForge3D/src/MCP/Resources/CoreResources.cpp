#include "MCP/Resources/CoreResources.h"

#include "MCP/McpStatus.h"
#include "MCP/ResourceRegistry.h"

namespace tf3d::mcp_layer
{

    void RegisterMcpCoreResources(ResourceRegistry &resources, ApplicationState *applicationState)
    {
        resources.Register({"terraforge://mcp/status",
                            "MCP status",
                            "Current TerraForge3D MCP bridge status.",
                            "application/json",
                            false,
                            [applicationState](const ResourceRequest &request) {
                                const nlohmann::json status = BuildMcpStatus(applicationState);
                                return McpResult::Success({{"uri", request.uri},
                                                           {"mimeType", "application/json"},
                                                           {"text", status.dump()}});
                            }});
    }

} // namespace tf3d::mcp_layer
