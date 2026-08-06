#include "MCP/Tools/ViewportTools.h"

#include "MCP/ActionRegistry.h"

#include "Base/FrameBuffer.h"
#include "Data/ApplicationState.h"
#include "Misc/ViewportManager.h"
#include "Renderer/RendererViewport.h"

#include "base64.hpp"
#include <stb/stb_image_write.h>

#include <cstdint>
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

        McpResult CaptureActiveViewport(ApplicationState *applicationState)
        {
            ViewportManager *viewport = FindActiveViewport(applicationState);
            if (viewport == nullptr || viewport->GetRendererViewport() == nullptr ||
                viewport->GetRendererViewport()->m_FrameBuffer == nullptr) {
                return McpResult::Failure(
                    "no_active_viewport",
                    "TerraForge3D does not currently have an active viewport.");
            }

            const auto &frameBuffer = viewport->GetRendererViewport()->m_FrameBuffer;
            const int width         = frameBuffer->GetWidth();
            const int height        = frameBuffer->GetHeight();
            std::vector<uint8_t> pixels;
            if (!frameBuffer->DownloadColorToU8(pixels)) {
                return McpResult::Failure(
                    "viewport_read_failed",
                    "TerraForge3D could not read the active viewport framebuffer.");
            }

            std::vector<uint8_t> jpeg;
            if (stbi_write_jpg_to_func(
                    AppendJpegData,
                    &jpeg,
                    width,
                    height,
                    3,
                    pixels.data(),
                    JpegQuality) == 0 ||
                jpeg.empty()) {
                return McpResult::Failure(
                    "jpeg_encode_failed",
                    "TerraForge3D could not encode the active viewport as JPEG.");
            }

            const std::string encoded = base64::encode(
                reinterpret_cast<const char *>(jpeg.data()), jpeg.size());

            nlohmann::json imageContent = nlohmann::json::array();
            imageContent.push_back({{"type", "image"},
                                    {"data", encoded},
                                    {"mimeType", "image/jpeg"}});
            return McpResult::SuccessWithToolContent(
                {{"mimeType", "image/jpeg"}, {"width", width}, {"height", height}},
                std::move(imageContent),
                "tf3d.viewport.capture");
        }
    } // namespace

    void RegisterMcpViewportTools(ActionRegistry &actions, ApplicationState *applicationState)
    {
        actions.Register({"tf3d.viewport.capture",
                          "Capture active viewport",
                          "Return the currently active TerraForge3D viewport as a JPEG image.",
                          nlohmann::json{
                              {"type", "object"},
                              {"properties", nlohmann::json::object()},
                              {"additionalProperties", false}},
                          nlohmann::json{{"readOnlyHint", true}},
                          ActionFlags::ReadOnly,
                          [applicationState](const nlohmann::json &) {
                              return CaptureActiveViewport(applicationState);
                          }});
    }

} // namespace tf3d::mcp_layer
