#pragma once

#include "API/OperationRegistry.h"

namespace tf3d::data
{
    class ApplicationState;
}

namespace tf3d::api
{

    void RegisterCoreOperations(OperationRegistry &operations, tf3d::data::ApplicationState *applicationState);

} // namespace tf3d::api
