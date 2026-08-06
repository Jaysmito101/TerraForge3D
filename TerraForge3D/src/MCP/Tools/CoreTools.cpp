#include "MCP/Tools/CoreTools.h"

#include "MCP/ActionRegistry.h"
#include "MCP/McpStatus.h"
#include "MCP/SchemaTemplate.h"

namespace tf3d::mcp_layer
{

    void RegisterMcpCoreTools(ActionRegistry &actions, ApplicationState *applicationState)
    {
        const McpSchemaTemplate schemaTemplates;
        const auto statusSchema = schemaTemplates.Compose("Tools/Core/Status.json");
        if (!statusSchema)
            return;

        actions.Register({"tf3d.mcp.status",
                          "MCP status",
                          "Return the TerraForge3D MCP bridge status and protocol revision.",
                          *statusSchema,
                          nlohmann::json{{"readOnlyHint", true}},
                          ActionFlags::ReadOnly,
                          [applicationState](const nlohmann::json &) {
                              return McpResult::Success(BuildMcpStatus(applicationState));
                          }});
    }

} // namespace tf3d::mcp_layer
