#include "MCP/Tools/ViewportTools.h"

#include "MCP/ActionRegistry.h"
#include "MCP/SchemaTemplate.h"
#include "MCP/ToolHelpers.h"

#include "Base/FrameBuffer.h"
#include "Exporters/Serializer.h"
#include "Data/ApplicationState.h"
#include "Misc/ViewportManager.h"
#include "Renderer/RendererViewport.h"

#include "base64.hpp"
#include <avir/avir.h>
#include <stb/stb_image_write.h>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tf3d::mcp_layer
{

    namespace
    {
        constexpr int JpegQuality = 90;

        void AppendJpegData(void *context, void *data, int size)
        {
            auto &jpeg        = *static_cast<std::vector<uint8_t> *>(context);
            const auto *bytes = static_cast<const uint8_t *>(data);
            jpeg.insert(jpeg.end(), bytes, bytes + size);
        }

        ViewportManager *FindFirstViewportWithRenderer(ApplicationState *applicationState)
        {
            if (applicationState == nullptr)
                return nullptr;

            for (ViewportManager *viewport : applicationState->viewportManagers) {
                if (viewport != nullptr && viewport->GetRendererViewport() != nullptr)
                    return viewport;
            }
            return nullptr;
        }

        bool ResolveViewport(ApplicationState *applicationState,
                             const nlohmann::json &arguments,
                             ViewportManager *&viewport,
                             McpResult &failure)
        {
            viewport = nullptr;
            if (applicationState == nullptr) {
                failure = McpResult::Failure(
                    McpErrorType::ViewportNotFound,
                    "TerraForge3D viewport state is unavailable.");
                return false;
            }

            if (!arguments.contains("ViewportId")) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "'ViewportId' is required.");
                return false;
            }

            const auto &id = arguments.at("ViewportId");
            if (!id.is_number_integer() || id.get<int64_t>() <= 0 ||
                id.get<int64_t>() > std::numeric_limits<int>::max()) {
                failure = McpResult::Failure(
                    McpErrorType::InvalidArguments,
                    "'ViewportId' must be a positive integer.");
                return false;
            }

            const int requestedId = id.get<int>();
            for (ViewportManager *candidate : applicationState->viewportManagers) {
                if (candidate != nullptr && static_cast<int>(candidate->GetID()) == requestedId) {
                    viewport = candidate;
                    return true;
                }
            }

            failure = McpResult::Failure(
                McpErrorType::ViewportNotFound,
                "The requested viewport does not exist.");
            return false;
        }

        McpResult UpdateViewportState(ViewportManager &manager,
                                      const nlohmann::json &state,
                                      const nlohmann::json &readOnlySchema)
        {
            if (!state.is_object())
                return McpResult::Failure(McpErrorType::InvalidArguments,
                                          "'State' must be an object.");

            if (manager.GetRendererViewport() == nullptr)
                return McpResult::Failure(McpErrorType::ViewportNotFound,
                                          "The requested viewport has no renderer state.");

            if (state.contains("Mode")) {
                if (!state.at("Mode").is_string())
                    return McpResult::Failure(McpErrorType::InvalidArguments,
                                              "'Mode' must be a string.");
                renderer::RendererViewportMode mode;
                if (!renderer::TryParseRendererViewportMode(state.at("Mode").get<std::string>(), mode))
                    return McpResult::Failure(McpErrorType::InvalidArguments,
                                              "'Mode' is not a supported viewport mode.");
            }

            if (const auto cameraState = state.find("Camera");
                cameraState != state.end() && !cameraState->is_object()) {
                return McpResult::Failure(McpErrorType::InvalidArguments,
                                          "'Camera' state must be an object.");
            }

            return tool_helpers::ApplySerializedState(
                state,
                &readOnlySchema,
                [&manager] { return manager.Save(); },
                [&manager](const auto &data) { manager.Load(data); });
        }

        McpResult CaptureViewport(ApplicationState *applicationState,
                                  const nlohmann::json &arguments)
        {
            int requestedMaxDimension = 0;
            if (arguments.contains("MaxDimension")) {
                const auto &maxDimension = arguments.at("MaxDimension");
                if (!maxDimension.is_number_integer()) {
                    return McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        "The optional 'MaxDimension' argument must be a positive integer.");
                }

                const int64_t requestedValue = maxDimension.get<int64_t>();
                if (requestedValue <= 0 ||
                    requestedValue > static_cast<int64_t>(std::numeric_limits<int>::max())) {
                    return McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        "The optional 'MaxDimension' argument must be a positive integer.");
                }
                requestedMaxDimension = static_cast<int>(requestedValue);
            }

            ViewportManager *viewport = nullptr;
            McpResult failure;
            if (!ResolveViewport(applicationState, arguments, viewport, failure))
                return failure;

            if (viewport->GetRendererViewport() == nullptr ||
                viewport->GetRendererViewport()->GetFrameBuffer() == nullptr) {
                return McpResult::Failure(
                    McpErrorType::ViewportNotFound,
                    "The requested viewport has no renderer framebuffer.");
            }

            const auto &frameBuffer = viewport->GetRendererViewport()->GetFrameBuffer();
            const int width         = frameBuffer->GetWidth();
            const int height        = frameBuffer->GetHeight();
            std::vector<uint8_t> pixels;
            if (!frameBuffer->DownloadColorToU8(pixels)) {
                return McpResult::Failure(
                    McpErrorType::ViewportReadFailed,
                    "TerraForge3D could not read the requested viewport framebuffer.");
            }

            int outputWidth  = width;
            int outputHeight = height;
            if (requestedMaxDimension > 0 && std::max(width, height) > requestedMaxDimension) {
                const double scale = static_cast<double>(requestedMaxDimension) /
                                     static_cast<double>(std::max(width, height));
                outputWidth  = std::max(1, static_cast<int>(std::lround(width * scale)));
                outputHeight = std::max(1, static_cast<int>(std::lround(height * scale)));

                std::vector<uint8_t> resized(
                    static_cast<size_t>(outputWidth) * static_cast<size_t>(outputHeight) * 3u);
                avir::CImageResizer<> imageResizer(8);
                imageResizer.resizeImage(
                    pixels.data(), width, height, 0,
                    resized.data(), outputWidth, outputHeight, 3, 0);
                pixels = std::move(resized);
            }

            std::vector<uint8_t> jpeg;
            if (stbi_write_jpg_to_func(
                    AppendJpegData,
                    &jpeg,
                    outputWidth,
                    outputHeight,
                    3,
                    pixels.data(),
                    JpegQuality) == 0 ||
                jpeg.empty()) {
                return McpResult::Failure(
                    McpErrorType::JpegEncodeFailed,
                    "TerraForge3D could not encode the requested viewport as JPEG.");
            }

            const std::string encoded = base64::encode(
                reinterpret_cast<const char *>(jpeg.data()), jpeg.size());

            nlohmann::json imageContent = nlohmann::json::array();
            imageContent.push_back({{"type", "image"},
                                    {"data", encoded},
                                    {"mimeType", "image/jpeg"}});
            return McpResult::SuccessWithToolContent(
                {{"mimeType", "image/jpeg"},
                 {"Width", outputWidth},
                 {"Height", outputHeight}},
                std::move(imageContent));
        }

        McpResult ListVisibleViewports(ApplicationState *applicationState)
        {
            if (applicationState == nullptr)
                return McpResult::Failure(
                    McpErrorType::ViewportNotFound,
                    "TerraForge3D viewport state is unavailable.");

            nlohmann::json viewports = nlohmann::json::array();
            for (ViewportManager *viewport : applicationState->viewportManagers) {
                if (viewport == nullptr || !viewport->IsVisible())
                    continue;

                nlohmann::json entry = {
                    {"ViewportId", static_cast<int>(viewport->GetID())},
                    {"Visible", true},
                    {"Active", viewport->IsActive()},
                    {"Display", {{"Width", viewport->GetDisplayWidth()}, {"Height", viewport->GetDisplayHeight()}}}};
                if (const auto *rendererViewport = viewport->GetRendererViewport()) {
                    entry["Mode"] = std::string(renderer::RendererViewportModeToString(
                        rendererViewport->GetMode()));
                }
                viewports.push_back(std::move(entry));
            }

            return McpResult::Success(
                {{"Viewports", std::move(viewports)}});
        }

        McpSchemaRuntimeProvider BuildViewportSchemaRuntime(ApplicationState *applicationState)
        {
            return [applicationState](std::string_view name) -> std::optional<nlohmann::json> {
                if (name == "Viewport.Modes") {
                    nlohmann::json modes = nlohmann::json::array();
                    for (int value = 0;
                         value < static_cast<int>(renderer::RendererViewportMode::Count);
                         ++value) {
                        modes.push_back(std::string(renderer::RendererViewportModeToString(
                            static_cast<renderer::RendererViewportMode>(value))));
                    }
                    return modes;
                }

                if (name == "Viewport.TextureSlotIndices") {
                    nlohmann::json slots = nlohmann::json::array();
                    for (int value = 0; value < 6; ++value)
                        slots.push_back(value);
                    return slots;
                }

                if (name == "Viewport.TextureChannelIndices") {
                    nlohmann::json channels   = nlohmann::json::array();
                    ViewportManager *viewport = FindFirstViewportWithRenderer(applicationState);
                    const int channelCount    = viewport == nullptr
                                                    ? 4
                                                    : static_cast<int>(viewport->GetRendererViewport()
                                                                           ->GetTextureSlotDetailed()
                                                                           .size());
                    for (int value = 0; value < channelCount; ++value)
                        channels.push_back(value);
                    return channels;
                }

                if (name == "Viewport.TextureChannelCount") {
                    ViewportManager *viewport = FindFirstViewportWithRenderer(applicationState);
                    return viewport == nullptr
                               ? 4
                               : static_cast<int>(viewport->GetRendererViewport()
                                                      ->GetTextureSlotDetailed()
                                                      .size());
                }

                return std::nullopt;
            };
        }
    } // namespace

    void RegisterMcpViewportTools(ActionRegistry &actions, ApplicationState *applicationState)
    {
        const McpSchemaRuntimeProvider runtime = BuildViewportSchemaRuntime(applicationState);
        const auto updateReadOnlySchema =
            McpSchemaTemplate::ComposeDefault("Tools/Viewport/UpdateStateReadOnly.json");

        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Viewport/Actions/List.json",
            [applicationState](const nlohmann::json &) {
                return ListVisibleViewports(applicationState);
            });

        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Viewport/Actions/GetState.json",
            [applicationState](const nlohmann::json &arguments) {
                ViewportManager *viewport = nullptr;
                McpResult failure;
                if (!ResolveViewport(applicationState, arguments, viewport, failure))
                    return failure;
                if (viewport->GetRendererViewport() == nullptr)
                    return McpResult::Failure(
                        McpErrorType::ViewportNotFound,
                        "The requested viewport has no renderer state.");
                return tool_helpers::GetSerializedState(
                    [viewport] { return viewport->Save(); });
            });

        if (updateReadOnlySchema) {
            TF3D_MCP_REGISTER_ACTION_WITH_RUNTIME(
                actions,
                "Tools/Viewport/Actions/UpdateState.json",
                runtime,
                [applicationState, readOnlySchema = *updateReadOnlySchema](
                    const nlohmann::json &arguments) {
                    if (!arguments.contains("State"))
                        return McpResult::Failure(
                            McpErrorType::InvalidArguments,
                            "'State' is required.");
                    ViewportManager *viewport = nullptr;
                    McpResult failure;
                    if (!ResolveViewport(applicationState, arguments, viewport, failure))
                        return failure;
                    return UpdateViewportState(*viewport, arguments.at("State"), readOnlySchema);
                });
        }

        TF3D_MCP_REGISTER_ACTION(
            actions,
            "Tools/Viewport/Actions/Capture.json",
            [applicationState](const nlohmann::json &arguments) {
                return CaptureViewport(applicationState, arguments);
            });
    }

} // namespace tf3d::mcp_layer
