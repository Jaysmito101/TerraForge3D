#include "API/Operations/SceneOperations.h"

#include "API/OperationHelpers.h"
#include "API/SchemaTemplate.h"

#include "Base/Logging/Logger.h"
#include "Data/ApplicationState.h"
#include "Renderer/RendererManager.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace tf3d::api
{

    namespace
    {
        renderer::RendererManager *ResolveRendererManager(
            tf3d::data::ApplicationState *applicationState,
            ApiResult &failure)
        {
            if (applicationState == nullptr || applicationState->rendererManager == nullptr) {
                failure = ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "TerraForge3D renderer settings are unavailable.");
                return nullptr;
            }
            return applicationState->rendererManager;
        }

        bool ValidateSkyMapPath(const std::string &path, ApiResult &failure)
        {
            if (path.empty()) {
                failure = ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "The sky map path cannot be empty.");
                return false;
            }

            std::error_code error;
            const std::filesystem::path filePath(path);
            if (!std::filesystem::is_regular_file(filePath, error) || error) {
                failure = ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "The sky map path must point to a readable file.",
                    {{"Path", path}});
                return false;
            }
            return true;
        }

        bool ReadSkyMapPath(const nlohmann::json &arguments,
                            std::string &path,
                            ApiResult &failure)
        {
            const auto value = arguments.find("Path");
            if (value == arguments.end()) {
                failure = ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "'Path' is required.");
                return false;
            }
            if (!value->is_string()) {
                failure = ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "'Path' must be a string.");
                return false;
            }

            path = value->get<std::string>();
            return ValidateSkyMapPath(path, failure);
        }

        ApiResult GetTerrainState(tf3d::data::ApplicationState *applicationState)
        {
            ApiResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr) {
                return failure;
            }
            return operation_helpers::GetSerializedState(
                [rendererManager] { return rendererManager->SaveTerrainSettings(); });
        }

        ApiResult UpdateTerrainState(tf3d::data::ApplicationState *applicationState,
                                     const nlohmann::json &arguments,
                                     const std::optional<nlohmann::json> &readOnlySchema)
        {
            ApiResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr) {
                return failure;
            }

            return operation_helpers::UpdateSerializedState(
                arguments,
                operation_helpers::SchemaPointer(readOnlySchema),
                [rendererManager] { return rendererManager->SaveTerrainSettings(); },
                [rendererManager](const auto &state) {
                    return rendererManager->LoadTerrainSettings(state);
                },
                "The requested terrain renderer settings were rejected.");
        }

        ApiResult GetLightsState(tf3d::data::ApplicationState *applicationState)
        {
            ApiResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr || rendererManager->GetRendererLights() == nullptr) {
                return failure.IsSuccess()
                           ? ApiResult::Failure(
                                 ApiErrorType::InvalidArguments,
                                 "TerraForge3D light settings are unavailable.")
                           : failure;
            }
            return operation_helpers::GetSerializedState(
                [rendererManager] { return rendererManager->GetRendererLights()->Save(); });
        }

        ApiResult GetSkyState(tf3d::data::ApplicationState *applicationState)
        {
            ApiResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr || rendererManager->GetSkyRenderer() == nullptr) {
                return failure.IsSuccess()
                           ? ApiResult::Failure(
                                 ApiErrorType::InvalidArguments,
                                 "TerraForge3D sky settings are unavailable.")
                           : failure;
            }
            return operation_helpers::GetSerializedState(
                [rendererManager] { return rendererManager->GetSkyRenderer()->Save(); });
        }

        ApiResult UpdateSkyState(tf3d::data::ApplicationState *applicationState,
                                 const nlohmann::json &arguments,
                                 const std::optional<nlohmann::json> &readOnlySchema)
        {
            const nlohmann::json *state = nullptr;
            ApiResult failure;
            if (!operation_helpers::ReadState(arguments, state, failure)) {
                return failure;
            }

            const auto path = state->find("SkyMapPath");
            if (path != state->end()) {
                if (!path->is_string()) {
                    return ApiResult::Failure(
                        ApiErrorType::InvalidArguments,
                        "'SkyMapPath' must be a string.");
                }
                if (!ValidateSkyMapPath(path->get<std::string>(), failure)) {
                    return failure;
                }
            }

            ApiResult rendererFailure;
            auto *rendererManager = ResolveRendererManager(applicationState, rendererFailure);
            if (rendererManager == nullptr) {
                return rendererFailure;
            }
            auto *sky = rendererManager->GetSkyRenderer();
            if (sky == nullptr) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "TerraForge3D sky settings are unavailable.");
            }

            return operation_helpers::ApplySerializedState(
                *state,
                operation_helpers::SchemaPointer(readOnlySchema),
                [sky] { return sky->Save(); },
                [sky](const auto &data) { return sky->Load(data); },
                "The requested renderer settings were rejected.");
        }

        ApiResult LoadSkyMap(tf3d::data::ApplicationState *applicationState,
                             const nlohmann::json &arguments)
        {
            std::string path;
            ApiResult failure;
            if (!ReadSkyMapPath(arguments, path, failure)) {
                return failure;
            }

            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr) {
                return failure;
            }
            auto *sky = rendererManager->GetSkyRenderer();
            if (sky == nullptr) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "TerraForge3D sky settings are unavailable.");
            }
            if (!sky->LoadSkyMap(path)) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "TerraForge3D could not load the requested sky map.",
                    {{"Path", path}});
            }
            return operation_helpers::GetSerializedState([sky] { return sky->Save(); });
        }

        ApiResult ReloadSkyShaders(tf3d::data::ApplicationState *applicationState)
        {
            ApiResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr) {
                return failure;
            }
            auto *sky = rendererManager->GetSkyRenderer();
            if (sky == nullptr) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "TerraForge3D sky settings are unavailable.");
            }
            sky->ReloadShaders();
            return operation_helpers::GetSerializedState([sky] { return sky->Save(); });
        }

        ApiResult GetSeaState(tf3d::data::ApplicationState *applicationState)
        {
            ApiResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr || rendererManager->GetSeaRenderer() == nullptr) {
                return failure.IsSuccess()
                           ? ApiResult::Failure(
                                 ApiErrorType::InvalidArguments,
                                 "TerraForge3D sea settings are unavailable.")
                           : failure;
            }
            return operation_helpers::GetSerializedState(
                [rendererManager] { return rendererManager->GetSeaRenderer()->Save(); });
        }

        ApiResult ReloadSeaShaders(tf3d::data::ApplicationState *applicationState)
        {
            ApiResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr) {
                return failure;
            }
            auto *sea = rendererManager->GetSeaRenderer();
            if (sea == nullptr) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "TerraForge3D sea settings are unavailable.");
            }
            sea->ReloadShaders();
            return operation_helpers::GetSerializedState([sea] { return sea->Save(); });
        }

        ApiResult ResetSeaSettings(tf3d::data::ApplicationState *applicationState)
        {
            ApiResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr) {
                return failure;
            }
            auto *sea = rendererManager->GetSeaRenderer();
            if (sea == nullptr) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "TerraForge3D sea settings are unavailable.");
            }
            if (!sea->ResetSettings()) {
                return ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "Reset is not enabled for TerraForge3D sea settings.");
            }
            return operation_helpers::GetSerializedState([sea] { return sea->Save(); });
        }

        ApiSchemaRuntimeProvider BuildSceneSchemaRuntime(tf3d::data::ApplicationState *applicationState)
        {
            return [applicationState](std::string_view name) -> std::optional<nlohmann::json> {
                if (applicationState == nullptr || applicationState->rendererManager == nullptr) {
                    return std::nullopt;
                }

                if (name == "Sea.State") {
                    if (applicationState->rendererManager->GetSeaRenderer() == nullptr) {
                        return std::nullopt;
                    }
                    return applicationState->rendererManager->GetSeaRenderer()->GetSettingsSchema();
                }

                if (name == "Lights.State") {
                    if (applicationState->rendererManager->GetRendererLights() == nullptr) {
                        return std::nullopt;
                    }
                    return applicationState->rendererManager->GetRendererLights()->GetSettingsSchema();
                }

                if (name == "Sky.State") {
                    if (applicationState->rendererManager->GetSkyRenderer() == nullptr) {
                        return std::nullopt;
                    }
                    return applicationState->rendererManager->GetSkyRenderer()->GetSettingsSchema();
                }

                if (name == "Terrain.State") {
                    return applicationState->rendererManager->GetTerrainSettingsSchema();
                }

                return std::nullopt;
            };
        }
    } // namespace

    void RegisterSceneOperations(OperationRegistry &operations, tf3d::data::ApplicationState *applicationState)
    {
        const ApiSchemaRuntimeProvider runtime = BuildSceneSchemaRuntime(applicationState);
        const auto terrainReadOnlySchema =
            ApiSchemaTemplate::ComposeDefault("Operations/Terrain/UpdateStateReadOnly.json", runtime);
        const auto skyReadOnlySchema =
            ApiSchemaTemplate::ComposeDefault("Operations/Sky/UpdateStateReadOnly.json", runtime);

        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Terrain/Definitions/GetState.json",
            [applicationState](const nlohmann::json &) {
                return GetTerrainState(applicationState);
            });
        TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(
            operations,
            "Operations/Terrain/Definitions/UpdateState.json",
            runtime,
            [applicationState, terrainReadOnlySchema](const nlohmann::json &arguments) {
                return UpdateTerrainState(applicationState, arguments, terrainReadOnlySchema);
            });

        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Lights/Definitions/GetState.json",
            [applicationState](const nlohmann::json &) {
                return GetLightsState(applicationState);
            });
        TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(
            operations,
            "Operations/Lights/Definitions/UpdateState.json",
            runtime,
            [applicationState](const nlohmann::json &arguments) {
                ApiResult failure;
                auto *rendererManager = ResolveRendererManager(applicationState, failure);
                if (rendererManager == nullptr) {
                    return failure;
                }
                auto *lights = rendererManager->GetRendererLights();
                if (lights == nullptr) {
                    return ApiResult::Failure(
                        ApiErrorType::InvalidArguments,
                        "TerraForge3D light settings are unavailable.");
                }
                return operation_helpers::UpdateSerializedState(
                    arguments,
                    nullptr,
                    [lights] { return lights->Save(); },
                    [lights](const auto &state) { return lights->Load(state); });
            });

        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Sky/Definitions/GetState.json",
            [applicationState](const nlohmann::json &) {
                return GetSkyState(applicationState);
            });
        TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(
            operations,
            "Operations/Sky/Definitions/UpdateState.json",
            runtime,
            [applicationState, skyReadOnlySchema](const nlohmann::json &arguments) {
                return UpdateSkyState(applicationState, arguments, skyReadOnlySchema);
            });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Sky/Definitions/LoadMap.json",
            [applicationState](const nlohmann::json &arguments) {
                return LoadSkyMap(applicationState, arguments);
            });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Sky/Definitions/ReloadShaders.json",
            [applicationState](const nlohmann::json &) {
                return ReloadSkyShaders(applicationState);
            });

        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Sea/Definitions/GetState.json",
            [applicationState](const nlohmann::json &) {
                return GetSeaState(applicationState);
            });
        TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(
            operations,
            "Operations/Sea/Definitions/UpdateState.json",
            runtime,
            [applicationState](const nlohmann::json &arguments) {
                ApiResult failure;
                auto *rendererManager = ResolveRendererManager(applicationState, failure);
                if (rendererManager == nullptr) {
                    return failure;
                }
                auto *sea = rendererManager->GetSeaRenderer();
                if (sea == nullptr) {
                    return ApiResult::Failure(
                        ApiErrorType::InvalidArguments,
                        "TerraForge3D sea settings are unavailable.");
                }
                return operation_helpers::UpdateSerializedState(
                    arguments,
                    nullptr,
                    [sea] { return sea->Save(); },
                    [sea](const auto &state) { return sea->Load(state); });
            });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Sea/Definitions/ReloadShaders.json",
            [applicationState](const nlohmann::json &) {
                return ReloadSeaShaders(applicationState);
            });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Sea/Definitions/Reset.json",
            [applicationState](const nlohmann::json &) {
                return ResetSeaSettings(applicationState);
            });
    }

} // namespace tf3d::api
