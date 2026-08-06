#include "MCP/Tools/CoreTools.h"

#include "MCP/ActionRegistry.h"
#include "MCP/McpStatus.h"
#include "MCP/SchemaTemplate.h"

namespace tf3d::mcp_layer
{

    void RegisterMcpCoreTools(ActionRegistry &actions, ApplicationState *applicationState)
    {
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Core/Actions/Status.json"),
            [applicationState](const nlohmann::json &) {
                return McpResult::Success(BuildMcpStatus(applicationState));
            });
    }

} // namespace tf3d::mcp_layer
