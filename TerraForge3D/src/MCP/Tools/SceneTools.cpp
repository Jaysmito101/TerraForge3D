#include "MCP/Tools/SceneTools.h"

#include "MCP/SchemaTemplate.h"
#include "MCP/ToolHelpers.h"

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

        McpResult GetTerrainState(ApplicationState *applicationState)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr)
                return failure;
            return tool_helpers::GetSerializedState(
                [rendererManager] { return rendererManager->SaveTerrainSettings(); });
        }

        McpResult UpdateTerrainState(ApplicationState *applicationState,
                                     const nlohmann::json &arguments,
                                     const std::optional<nlohmann::json> &readOnlySchema)
        {
            McpResult failure;
            auto *rendererManager = ResolveRendererManager(applicationState, failure);
            if (rendererManager == nullptr)
                return failure;

            return tool_helpers::UpdateSerializedState(
                arguments,
                tool_helpers::SchemaPointer(readOnlySchema),
                [rendererManager] { return rendererManager->SaveTerrainSettings(); },
                [rendererManager](const auto &state) {
                    return rendererManager->LoadTerrainSettings(state);
                },
                "The requested terrain renderer settings were rejected.");
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
            return tool_helpers::GetSerializedState(
                [rendererManager] { return rendererManager->GetRendererLights()->Save(); });
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
            return tool_helpers::GetSerializedState(
                [rendererManager] { return rendererManager->GetSkyRenderer()->Save(); });
        }

        McpResult UpdateSkyState(ApplicationState *applicationState,
                                 const nlohmann::json &arguments,
                                 const std::optional<nlohmann::json> &readOnlySchema)
        {
            const nlohmann::json *state = nullptr;
            McpResult failure;
            if (!tool_helpers::ReadState(arguments, state, failure))
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

            return tool_helpers::ApplySerializedState(
                *state,
                tool_helpers::SchemaPointer(readOnlySchema),
                [sky] { return sky->Save(); },
                [sky](const auto &data) { return sky->Load(data); },
                "The requested renderer settings were rejected.");
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
            return tool_helpers::GetSerializedState([sky] { return sky->Save(); });
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
            return tool_helpers::GetSerializedState([sky] { return sky->Save(); });
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
            return tool_helpers::GetSerializedState(
                [rendererManager] { return rendererManager->GetSeaRenderer()->Save(); });
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
            return tool_helpers::GetSerializedState([sea] { return sea->Save(); });
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
            return tool_helpers::GetSerializedState([sea] { return sea->Save(); });
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

        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Terrain/Actions/GetState.json",
            [applicationState](const nlohmann::json &) {
                return GetTerrainState(applicationState);
            });
        TF3D_MCP_REGISTER_ACTION_WITH_RUNTIME(
            actions,
            "Tools/Terrain/Actions/UpdateState.json",
            runtime,
            [applicationState, terrainReadOnlySchema](const nlohmann::json &arguments) {
                return UpdateTerrainState(applicationState, arguments, terrainReadOnlySchema);
            });

        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Lights/Actions/GetState.json",
            [applicationState](const nlohmann::json &) {
                return GetLightsState(applicationState);
            });
        TF3D_MCP_REGISTER_ACTION_WITH_RUNTIME(
            actions,
            "Tools/Lights/Actions/UpdateState.json",
            runtime,
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
                return tool_helpers::UpdateSerializedState(
                    arguments,
                    nullptr,
                    [lights] { return lights->Save(); },
                    [lights](const auto &state) { return lights->Load(state); });
            });

        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Sky/Actions/GetState.json",
            [applicationState](const nlohmann::json &) {
                return GetSkyState(applicationState);
            });
        TF3D_MCP_REGISTER_ACTION_WITH_RUNTIME(
            actions,
            "Tools/Sky/Actions/UpdateState.json",
            runtime,
            [applicationState, skyReadOnlySchema](const nlohmann::json &arguments) {
                return UpdateSkyState(applicationState, arguments, skyReadOnlySchema);
            });
        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Sky/Actions/LoadMap.json",
            [applicationState](const nlohmann::json &arguments) {
                return LoadSkyMap(applicationState, arguments);
            });
        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Sky/Actions/ReloadShaders.json",
            [applicationState](const nlohmann::json &) {
                return ReloadSkyShaders(applicationState);
            });

        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Sea/Actions/GetState.json",
            [applicationState](const nlohmann::json &) {
                return GetSeaState(applicationState);
            });
        TF3D_MCP_REGISTER_ACTION_WITH_RUNTIME(
            actions,
            "Tools/Sea/Actions/UpdateState.json",
            runtime,
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
                return tool_helpers::UpdateSerializedState(
                    arguments,
                    nullptr,
                    [sea] { return sea->Save(); },
                    [sea](const auto &state) { return sea->Load(state); });
            });
        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Sea/Actions/ReloadShaders.json",
            [applicationState](const nlohmann::json &) {
                return ReloadSeaShaders(applicationState);
            });
        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Sea/Actions/Reset.json",
            [applicationState](const nlohmann::json &) {
                return ResetSeaSettings(applicationState);
            });
    }

} // namespace tf3d::mcp_layer
