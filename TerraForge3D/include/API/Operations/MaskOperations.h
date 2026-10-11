#pragma once

namespace tf3d::data
{
    class ApplicationState;
}

namespace tf3d::api
{

    class OperationRegistry;

    void RegisterMaskOperations(OperationRegistry &operations,
                                tf3d::data::ApplicationState *applicationState);

} // namespace tf3d::api
