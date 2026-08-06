#pragma once

#include "MCP/ActionRegistry.h"

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::mcp_layer
{
    void RegisterMcpSceneTools(ActionRegistry &actions, ApplicationState *applicationState);
}
