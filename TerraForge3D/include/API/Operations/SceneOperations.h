#pragma once

#include "API/OperationRegistry.h"

namespace tf3d::data
{
    class ApplicationState;
}

namespace tf3d::api
{

    void RegisterSceneOperations(OperationRegistry &operations, tf3d::data::ApplicationState *applicationState);
}
