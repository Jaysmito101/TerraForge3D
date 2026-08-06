#pragma once

#include <nlohmann/json.hpp>

namespace tf3d::data
{
    class ApplicationState;
}
using tf3d::data::ApplicationState;

namespace tf3d::mcp_layer
{

    nlohmann::json BuildMcpStatus(const ApplicationState *applicationState);

} // namespace tf3d::mcp_layer
