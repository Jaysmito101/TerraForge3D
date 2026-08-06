#include "MCP/Tools/ViewportTools.h"

#include "MCP/ActionRegistry.h"

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

        ViewportManager *FindActiveViewport(ApplicationState *applicationState)
        {
            if (applicationState == nullptr)
                return nullptr;

            for (ViewportManager *viewport : applicationState->viewportManagers) {
                if (viewport != nullptr && viewport->IsActive()) {
                    return viewport;
                }
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
                    McpErrorType::NoActiveViewport,
                    "TerraForge3D does not currently have an active viewport.");
                return false;
            }

            int requestedId = 0;
            if (arguments.contains("viewportId")) {
                const auto &id = arguments.at("viewportId");
                if (!id.is_number_integer() || id.get<int64_t>() <= 0 ||
                    id.get<int64_t>() > std::numeric_limits<int>::max()) {
                    failure = McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        "'viewportId' must be a positive integer.");
                    return false;
                }
                requestedId = id.get<int>();
            }

            if (requestedId > 0) {
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

            viewport = FindActiveViewport(applicationState);
            if (viewport != nullptr)
                return true;
            failure = McpResult::Failure(
                McpErrorType::NoActiveViewport,
                "TerraForge3D does not currently have an active viewport.");
            return false;
        }

        McpResult UpdateViewportState(ViewportManager &manager,
                                      const nlohmann::json &state)
        {
            if (!state.is_object())
                return McpResult::Failure(McpErrorType::InvalidArguments,
                                          "'state' must be an object.");

            if (manager.GetRendererViewport() == nullptr)
                return McpResult::Failure(McpErrorType::ViewportNotFound,
                                          "The requested viewport has no renderer state.");

            if (const auto cameraState = state.value("camera", nlohmann::json::object());
                !cameraState.is_object()) {
                return McpResult::Failure(McpErrorType::InvalidArguments,
                                          "'camera' state must be an object.");
            } else {
                if (cameraState.contains("CameraID") || cameraState.contains("Perspective") ||
                    cameraState.contains("AutomaticClipping") || cameraState.contains("NearClip") ||
                    cameraState.contains("FarClip") || cameraState.contains("AspectRatio") ||
                    cameraState.contains("Position") || cameraState.contains("EffectiveNearClip") ||
                    cameraState.contains("EffectiveFarClip")) {
                    return McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        "Camera projection, clipping, and derived position fields are read-only.");
                }
            }

            const SerializerNode current = manager.Save();
            const SerializerNode updates = CreateSerializerNodeFromJson(state);
            current->Merge(*updates);
            manager.Load(current);
            return McpResult::Success(manager.Save()->ToJson(), "tf3d.viewport.update_state");
        }

        McpResult CaptureActiveViewport(ApplicationState *applicationState,
                                        const nlohmann::json &arguments)
        {
            int requestedMaxDimension = 0;
            if (arguments.contains("maxDimension")) {
                const auto &maxDimension = arguments.at("maxDimension");
                if (!maxDimension.is_number_integer()) {
                    return McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        "The optional 'maxDimension' argument must be a positive integer.");
                }

                const int64_t requestedValue = maxDimension.get<int64_t>();
                if (requestedValue <= 0 ||
                    requestedValue > static_cast<int64_t>(std::numeric_limits<int>::max())) {
                    return McpResult::Failure(
                        McpErrorType::InvalidArguments,
                        "The optional 'maxDimension' argument must be a positive integer.");
                }
                requestedMaxDimension = static_cast<int>(requestedValue);
            }

            ViewportManager *viewport = FindActiveViewport(applicationState);
            if (viewport == nullptr || viewport->GetRendererViewport() == nullptr ||
                viewport->GetRendererViewport()->GetFrameBuffer() == nullptr) {
                return McpResult::Failure(
                    McpErrorType::NoActiveViewport,
                    "TerraForge3D does not currently have an active viewport.");
            }

            const auto &frameBuffer = viewport->GetRendererViewport()->GetFrameBuffer();
            const int width         = frameBuffer->GetWidth();
            const int height        = frameBuffer->GetHeight();
            std::vector<uint8_t> pixels;
            if (!frameBuffer->DownloadColorToU8(pixels)) {
                return McpResult::Failure(
                    McpErrorType::ViewportReadFailed,
                    "TerraForge3D could not read the active viewport framebuffer.");
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
                    "TerraForge3D could not encode the active viewport as JPEG.");
            }

            const std::string encoded = base64::encode(
                reinterpret_cast<const char *>(jpeg.data()), jpeg.size());

            nlohmann::json imageContent = nlohmann::json::array();
            imageContent.push_back({{"type", "image"},
                                    {"data", encoded},
                                    {"mimeType", "image/jpeg"}});
            return McpResult::SuccessWithToolContent(
                {{"mimeType", "image/jpeg"},
                 {"width", outputWidth},
                 {"height", outputHeight}},
                std::move(imageContent),
                "tf3d.viewport.capture");
        }
    } // namespace

    void RegisterMcpViewportTools(ActionRegistry &actions, ApplicationState *applicationState)
    {
        const nlohmann::json viewportIdProperty = {
            {"type", "integer"},
            {"minimum", 1},
            {"description", "Optional viewport ID. Omit it to use the active viewport."}};

        const nlohmann::json vector2UpdateSchema = {
            {"type", "object"},
            {"properties", {{"x", {{"type", "number"}}}, {"y", {{"type", "number"}}}}},
            {"required", {"x", "y"}},
            {"additionalProperties", false}};

        const nlohmann::json vector3UpdateSchema = {
            {"type", "object"},
            {"properties", {{"x", {{"type", "number"}}}, {"y", {{"type", "number"}}}, {"z", {{"type", "number"}}}}},
            {"required", {"x", "y", "z"}},
            {"additionalProperties", false}};

        const nlohmann::json textureChannelSchema = {
            {"type", "object"},
            {"properties", {{"textureSlot", {{"type", "integer"}, {"minimum", 0}, {"maximum", 5}}}, {"channel", {{"type", "integer"}, {"minimum", 0}, {"maximum", 3}}}}},
            {"required", {"textureSlot", "channel"}},
            {"additionalProperties", false}};

        nlohmann::json stateSchema = {
            {"type", "object"},
            {"properties", nlohmann::json::object()},
            {"additionalProperties", false}};
        stateSchema["properties"]["viewport"] = {
            {"type", "object"},
            {"properties", {{"mode", {{"type", "string"}, {"enum", {"Object", "Wireframe", "Heightmap", "TextureSlot"}}}}, {"visible", {{"type", "boolean"}}}, {"controlEnabled", {{"type", "boolean"}}}, {"autoCalculateAspectRatio", {{"type", "boolean"}}}}},
            {"additionalProperties", false}};
        stateSchema["properties"]["camera"] = {
            {"type", "object"},
            {"properties", {{"Target", vector3UpdateSchema}, {"Distance", {{"type", "number"}, {"exclusiveMinimum", 0.0}}}, {"Azimuth", {{"type", "number"}}}, {"Elevation", {{"type", "number"}}}, {"FieldOfView", {{"type", "number"}, {"exclusiveMinimum", 0.0}}}}},
            {"additionalProperties", false}};
        stateSchema["properties"]["modes"] = {
            {"type", "object"},
            {"properties", nlohmann::json::object()},
            {"additionalProperties", false}};
        stateSchema["properties"]["modes"]["properties"]["heightmap"] = {
            {"type", "object"},
            {"properties", {{"offset", vector2UpdateSchema}, {"scale", {{"type", "number"}, {"exclusiveMinimum", 0.0}}}}},
            {"additionalProperties", false}};
        stateSchema["properties"]["modes"]["properties"]["textureSlot"] = {
            {"type", "object"},
            {"properties", {{"offset", vector2UpdateSchema}, {"scale", {{"type", "number"}, {"exclusiveMinimum", 0.0}}}, {"detailedMode", {{"type", "boolean"}}}, {"textureSlot", {{"type", "integer"}, {"minimum", 0}, {"maximum", 5}}}, {"channels", {{"type", "array"}, {"minItems", 4}, {"maxItems", 4}, {"items", textureChannelSchema}}}}},
            {"additionalProperties", false}};

        actions.Register({"tf3d.viewport.get_state",
                          "Get viewport state",
                          "Return the selected viewport's complete structured state. "
                          "The state includes viewport layout and lifecycle flags, current mode, "
                          "camera settings, heightmap controls, texture-slot controls, and interaction state. "
                          "Omit viewportId to inspect the active viewport.",
                          nlohmann::json{
                              {"type", "object"},
                              {"properties", {{"viewportId", viewportIdProperty}}},
                              {"additionalProperties", false}},
                          nlohmann::json{{"readOnlyHint", true}},
                          ActionFlags::ReadOnly,
                          [applicationState](const nlohmann::json &arguments) {
                              ViewportManager *viewport = nullptr;
                              McpResult failure;
                              if (!ResolveViewport(applicationState, arguments, viewport, failure))
                                  return failure;
                              if (viewport->GetRendererViewport() == nullptr)
                                  return McpResult::Failure(McpErrorType::ViewportNotFound,
                                                            "The requested viewport has no renderer state.");
                              return McpResult::Success(viewport->Save()->ToJson(),
                                                        "tf3d.viewport.get_state");
                          }});

        actions.Register({"tf3d.viewport.update_state",
                          "Update viewport state",
                          "Update selected viewport state using a partial hierarchical object. "
                          "You can change viewport mode and controls, camera settings, heightmap pan/zoom, "
                          "or texture-slot settings. Omit viewportId to update the active viewport. "
                          "Read-only fields such as camera projection/clipping settings, render size, "
                          "mouse position, and terrain hit position are returned by get_state but are "
                          "not accepted here.",
                          nlohmann::json{
                              {"type", "object"},
                              {"properties", {{"viewportId", viewportIdProperty}, {"state", stateSchema}}},
                              {"required", {"state"}},
                              {"additionalProperties", false}},
                          nlohmann::json{{"readOnlyHint", false}},
                          ActionFlags::None,
                          [applicationState](const nlohmann::json &arguments) {
                              if (!arguments.contains("state"))
                                  return McpResult::Failure(McpErrorType::InvalidArguments,
                                                            "'state' is required.");
                              ViewportManager *viewport = nullptr;
                              McpResult failure;
                              if (!ResolveViewport(applicationState, arguments, viewport, failure))
                                  return failure;
                              return UpdateViewportState(*viewport, arguments.at("state"));
                          }});

        actions.Register({"tf3d.viewport.capture",
                          "Capture active viewport",
                          "Return the currently active TerraForge3D viewport as a JPEG image. "
                          "Optionally pass maxDimension to downscale the image before encoding, "
                          "which reduces image size and token usage; the aspect ratio is preserved "
                          "and the image is never upscaled.",
                          nlohmann::json{
                              {"type", "object"},
                              {"properties", nlohmann::json{
                                                 {"maxDimension", nlohmann::json{
                                                                      {"type", "integer"},
                                                                      {"minimum", 1},
                                                                      {"description", "Optional maximum width or height in pixels. "
                                                                                      "Use this to reduce image size and token usage."}}}}},
                              {"additionalProperties", false}},
                          nlohmann::json{{"readOnlyHint", true}}, ActionFlags::ReadOnly, [applicationState](const nlohmann::json &arguments) {
                              return CaptureActiveViewport(applicationState, arguments);
                          }});
    }

} // namespace tf3d::mcp_layer
