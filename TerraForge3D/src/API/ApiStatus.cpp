#include "API/ApiStatus.h"

#include "Data/VersionInfo.h"

namespace tf3d::api
{

    nlohmann::json BuildApiStatus(const tf3d::data::ApplicationState *applicationState)
    {
        return {
            {"applicationAttached", applicationState != nullptr},
            {"version", TERR3D_VERSION_STRING},
            {"apiVersion", "v1"},
            {"transport", "http"}};
    }

} // namespace tf3d::api
