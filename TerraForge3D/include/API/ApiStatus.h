#pragma once

#include <nlohmann/json.hpp>

namespace tf3d::data
{
    class ApplicationState;
}

namespace tf3d::api
{

    nlohmann::json BuildApiStatus(const tf3d::data::ApplicationState *applicationState);

} // namespace tf3d::api
