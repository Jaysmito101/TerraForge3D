#include "MCP/Tools/CoreTools.h"

#include "MCP/ActionRegistry.h"
#include "MCP/McpStatus.h"

void RegisterMcpCoreTools(ActionRegistry &actions, ApplicationState *applicationState)
{
    actions.Register({"tf3d.mcp.status",
                      "MCP status",
                      "Return the TerraForge3D MCP bridge status and protocol revision.",
                      nlohmann::json{
                          {"type", "object"},
                          {"properties", nlohmann::json::object()}},
                      nlohmann::json{{"readOnlyHint", true}},
                      ActionFlags::ReadOnly,
                      [applicationState](const nlohmann::json &) {
                          return McpResult::Success(BuildMcpStatus(applicationState));
                      }});
}
