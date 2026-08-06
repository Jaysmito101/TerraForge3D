#pragma once

#include "MCP/ResourceRegistry.h"

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::mcp_layer
{

    void RegisterMcpCoreResources(ResourceRegistry &resources, ApplicationState *applicationState);

} // namespace tf3d::mcp_layer
