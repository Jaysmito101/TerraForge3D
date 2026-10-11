#include "API/Operations/CoreOperations.h"

#include "API/ApiStatus.h"
#include "API/OperationRegistry.h"
#include "API/SchemaTemplate.h"

namespace tf3d::api
{

    void RegisterCoreOperations(OperationRegistry &operations, tf3d::data::ApplicationState *applicationState)
    {
        RegisterOperationFromJson(
            operations,
            ApiSchemaTemplate::ComposeDefault("Operations/Core/Definitions/Status.json"),
            [applicationState](const nlohmann::json &) {
                return ApiResult::Success(BuildApiStatus(applicationState));
            });
    }

} // namespace tf3d::api
