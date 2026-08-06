#include "MCP/Tools/ViewportTools.h"

#include "MCP/ActionRegistry.h"

#include "Base/FrameBuffer.h"
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
                viewport->GetRendererViewport()->m_FrameBuffer == nullptr) {
                return McpResult::Failure(
                    McpErrorType::NoActiveViewport,
                    "TerraForge3D does not currently have an active viewport.");
            }

            const auto &frameBuffer = viewport->GetRendererViewport()->m_FrameBuffer;
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
                outputWidth        = std::max(1, static_cast<int>(std::lround(width * scale)));
                outputHeight       = std::max(1, static_cast<int>(std::lround(height * scale)));

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
                          nlohmann::json{{"readOnlyHint", true}},
                          ActionFlags::ReadOnly,
                          [applicationState](const nlohmann::json &arguments) {
                              return CaptureActiveViewport(applicationState, arguments);
                          }});
    }

} // namespace tf3d::mcp_layer
