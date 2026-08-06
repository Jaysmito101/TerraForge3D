#include "MCP/Tools/SceneTools.h"

#include "MCP/SchemaTemplate.h"

#include "Base/Logging/Logger.h"
#include "Data/ApplicationState.h"
#include "Renderer/RendererManager.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace tf3d::mcp_layer
{

    namespace
    {
        renderer::RendererManager *ResolveRendererManager(
            ApplicationState *applicationState,
            McpResult &failure)
        {
            if (applicationState == nullptr || applicationState->rendererManager == nullptr) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "TerraForge3D renderer settings are unavailable.");
                return nullptr;
            }
            return applicationState->rendererManager;
        }

        bool ReadState(const nlohmann::json &arguments,
                       const nlohmann::json *&state,
                       McpResult &failure)
        {
            const auto value = arguments.find("State");
            if (value == arguments.end()) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "'State' is required.");
                return false;
            }
            if (!value->is_object()) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "'State' must be an object.");
                return false;
            }
            state = &*value;
            return true;
        }

        bool ValidateSkyMapPath(const std::string &path, McpResult &failure)
        {
            if (path.empty()) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "The sky map path cannot be empty.");
                return false;
            }

            std::error_code error;
            const std::filesystem::path filePath(path);
            if (!std::filesystem::is_regular_file(filePath, error) || error) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "The sky map path must point to a readable file.",
                    {{"Path", path}});
                return false;
            }
            return true;
        }

        bool ReadSkyMapPath(const nlohmann::json &arguments,
                            std::string &path,
                            McpResult &failure)
        {
            const auto value = arguments.find("Path");
            if (value == arguments.end()) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "'Path' is required.");
                return false;
            }
            if (!value->is_string()) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "'Path' must be a string.");
                return false;
            }

            path = value->get<std::string>();
            return ValidateSkyMapPath(path, failure);
        }

        template <typename Settings>
        McpResult UpdateSerializedSettings(
            Settings &settings,
            const nlohmann::json &arguments,
            const std::optional<nlohmann::json> &readOnlySchema = std::nullopt)
        {
            const nlohmann::json *state = nullptr;
            McpResult failure;
            if (!ReadState(arguments, state, failure))
                return failure;

            if (readOnlySchema) {
                std::string readOnlyError;
                if (!McpSchemaTemplate::ValidateWritable(*state, *readOnlySchema, readOnlyError))
                    return McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        readOnlyError.empty() ? "State contains a read-only field." : readOnlyError);
            }

            const exporters::SerializerNode current = settings.Save();
            const exporters::SerializerNode updates = exporters::CreateSerializerNodeFromJson(*state);
            current->Merge(*updates);
            if (!settings.Load(current))
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "The requested renderer settings were rejected.");

            return McpResult::Success(settings.Save()->ToJson());
        }

        McpResult GetTerrainState(ApplicationState *applicationState)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr)
                return failure;
            return McpResult::Success(rendererManager->SaveTerrainSettings()->ToJson());
        }

        McpResult UpdateTerrainState(ApplicationState *applicationState,
                                     const nlohmann::json &arguments,
                                     const std::optional<nlohmann::json> &readOnlySchema)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr)
                return failure;

            const nlohmann::json *state = nullptr;
            if (!ReadState(arguments, state, failure))
                return failure;

            std::string readOnlyError;
            if (readOnlySchema && !McpSchemaTemplate::ValidateWritable(
                                      *state, *readOnlySchema, readOnlyError))
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    readOnlyError.empty() ? "State contains a read-only field." : readOnlyError);

            const auto current = rendererManager->SaveTerrainSettings();
            const auto updates = exporters::CreateSerializerNodeFromJson(*state);
            current->Merge(*updates);
            if (!rendererManager->LoadTerrainSettings(current))
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "The requested terrain renderer settings were rejected.");
            return McpResult::Success(rendererManager->SaveTerrainSettings()->ToJson());
        }

        McpResult GetLightsState(ApplicationState *applicationState)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr || rendererManager->GetRendererLights() == nullptr)
                return failure.ok
                           ? McpResult::Failure(
                                 McpErrorType::InvalidArguments,
                                 "TerraForge3D light settings are unavailable.")
                           : failure;
            return McpResult::Success(rendererManager->GetRendererLights()->Save()->ToJson());
        }

        McpResult GetSkyState(ApplicationState *applicationState)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr || rendererManager->GetSkyRenderer() == nullptr)
                return failure.ok
                           ? McpResult::Failure(
                                 McpErrorType::InvalidArguments,
                                 "TerraForge3D sky settings are unavailable.")
                           : failure;
            return McpResult::Success(rendererManager->GetSkyRenderer()->Save()->ToJson());
        }

        McpResult UpdateSkyState(ApplicationState *applicationState,
                                 const nlohmann::json &arguments,
                                 const std::optional<nlohmann::json> &readOnlySchema)
        {
            const nlohmann::json *state = nullptr;
            McpResult failure;
            if (!ReadState(arguments, state, failure))
                return failure;

            const auto path = state->find("SkyMapPath");
            if (path != state->end()) {
                if (!path->is_string())
                    return McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        "'SkyMapPath' must be a string.");
                if (!ValidateSkyMapPath(path->get<std::string>(), failure))
                    return failure;
            }

            McpResult rendererFailure;
            auto *rendererManager = ResolveRendererManager(applicationState, rendererFailure);
            if (rendererManager == nullptr)
                return rendererFailure;
            auto *sky = rendererManager->GetSkyRenderer();
            if (sky == nullptr)
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "TerraForge3D sky settings are unavailable.");

            return UpdateSerializedSettings(*sky, arguments, readOnlySchema);
        }

        McpResult LoadSkyMap(ApplicationState *applicationState,
                             const nlohmann::json &arguments)
        {
            std::string path;
            McpResult failure;
            if (!ReadSkyMapPath(arguments, path, failure))
                return failure;

            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr)
                return failure;
            auto *sky = rendererManager->GetSkyRenderer();
            if (sky == nullptr)
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "TerraForge3D sky settings are unavailable.");
            if (!sky->LoadSkyMap(path))
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "TerraForge3D could not load the requested sky map.",
                    {{"Path", path}});
            return McpResult::Success(sky->Save()->ToJson());
        }

        McpResult ReloadSkyShaders(ApplicationState *applicationState)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr)
                return failure;
            auto *sky = rendererManager->GetSkyRenderer();
            if (sky == nullptr)
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "TerraForge3D sky settings are unavailable.");
            sky->ReloadShaders();
            return McpResult::Success(sky->Save()->ToJson());
        }

        McpResult GetSeaState(ApplicationState *applicationState)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr || rendererManager->GetSeaRenderer() == nullptr)
                return failure.ok
                           ? McpResult::Failure(
                                 McpErrorType::InvalidArguments,
                                 "TerraForge3D sea settings are unavailable.")
                           : failure;
            return McpResult::Success(rendererManager->GetSeaRenderer()->Save()->ToJson());
        }

        McpResult ReloadSeaShaders(ApplicationState *applicationState)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr)
                return failure;
            auto *sea = rendererManager->GetSeaRenderer();
            if (sea == nullptr)
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "TerraForge3D sea settings are unavailable.");
            sea->ReloadShaders();
            return McpResult::Success(sea->Save()->ToJson());
        }

        McpResult ResetSeaSettings(ApplicationState *applicationState)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr)
                return failure;
            auto *sea = rendererManager->GetSeaRenderer();
            if (sea == nullptr)
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "TerraForge3D sea settings are unavailable.");
            if (!sea->ResetSettings())
                return McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "Reset is not enabled for TerraForge3D sea settings.");
            return McpResult::Success(sea->Save()->ToJson());
        }

        McpSchemaRuntimeProvider BuildSceneSchemaRuntime(ApplicationState *applicationState)
        {
            return [applicationState](std::string_view name) -> std::optional<nlohmann::json> {
                if (applicationState == nullptr || applicationState->rendererManager == nullptr)
                    return std::nullopt;

                if (name == "Sea.State") {
                    if (applicationState->rendererManager->GetSeaRenderer() == nullptr)
                        return std::nullopt;
                    return applicationState->rendererManager->GetSeaRenderer()->GetSettingsSchema();
                }

                if (name == "Lights.State") {
                    if (applicationState->rendererManager->GetRendererLights() == nullptr)
                        return std::nullopt;
                    return applicationState->rendererManager->GetRendererLights()->GetSettingsSchema();
                }

                if (name == "Sky.State") {
                    if (applicationState->rendererManager->GetSkyRenderer() == nullptr)
                        return std::nullopt;
                    return applicationState->rendererManager->GetSkyRenderer()->GetSettingsSchema();
                }

                if (name == "Terrain.State")
                    return applicationState->rendererManager->GetTerrainSettingsSchema();

                return std::nullopt;
            };
        }
    } // namespace

    void RegisterMcpSceneTools(ActionRegistry &actions, ApplicationState *applicationState)
    {
        const McpSchemaRuntimeProvider runtime = BuildSceneSchemaRuntime(applicationState);
        const auto terrainReadOnlySchema =
            McpSchemaTemplate::ComposeDefault("Tools/Terrain/UpdateStateReadOnly.json", runtime);
        const auto skyReadOnlySchema =
            McpSchemaTemplate::ComposeDefault("Tools/Sky/UpdateStateReadOnly.json", runtime);

        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Terrain/Actions/GetState.json"),
            [applicationState](const nlohmann::json &) {
                return GetTerrainState(applicationState);
            });
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Terrain/Actions/UpdateState.json", runtime),
            [applicationState, terrainReadOnlySchema](const nlohmann::json &arguments) {
                return UpdateTerrainState(applicationState, arguments, terrainReadOnlySchema);
            });

        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Lights/Actions/GetState.json"),
            [applicationState](const nlohmann::json &) {
                return GetLightsState(applicationState);
            });
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Lights/Actions/UpdateState.json", runtime),
            [applicationState](const nlohmann::json &arguments) {
                McpResult failure;
                auto *rendererManager = ResolveRendererManager(applicationState, failure);
                if (rendererManager == nullptr)
                    return failure;
                auto *lights = rendererManager->GetRendererLights();
                if (lights == nullptr)
                    return McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        "TerraForge3D light settings are unavailable.");
                return UpdateSerializedSettings(*lights, arguments);
            });

        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Sky/Actions/GetState.json"),
            [applicationState](const nlohmann::json &) {
                return GetSkyState(applicationState);
            });
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Sky/Actions/UpdateState.json", runtime),
            [applicationState, skyReadOnlySchema](const nlohmann::json &arguments) {
                return UpdateSkyState(applicationState, arguments, skyReadOnlySchema);
            });
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Sky/Actions/LoadMap.json"),
            [applicationState](const nlohmann::json &arguments) {
                return LoadSkyMap(applicationState, arguments);
            });
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Sky/Actions/ReloadShaders.json"),
            [applicationState](const nlohmann::json &) {
                return ReloadSkyShaders(applicationState);
            });

        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Sea/Actions/GetState.json"),
            [applicationState](const nlohmann::json &) {
                return GetSeaState(applicationState);
            });
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Sea/Actions/UpdateState.json", runtime),
            [applicationState](const nlohmann::json &arguments) {
                McpResult failure;
                auto *rendererManager = ResolveRendererManager(applicationState, failure);
                if (rendererManager == nullptr)
                    return failure;
                auto *sea = rendererManager->GetSeaRenderer();
                if (sea == nullptr)
                    return McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        "TerraForge3D sea settings are unavailable.");
                return UpdateSerializedSettings(*sea, arguments);
            });
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Sea/Actions/ReloadShaders.json"),
            [applicationState](const nlohmann::json &) {
                return ReloadSeaShaders(applicationState);
            });
        RegisterActionFromJson(
            actions,
            McpSchemaTemplate::ComposeDefault("Tools/Sea/Actions/Reset.json"),
            [applicationState](const nlohmann::json &) {
                return ResetSeaSettings(applicationState);
            });
    }

} // namespace tf3d::mcp_layer
