#include "API/Operations/ViewportOperations.h"

#include "API/OperationHelpers.h"
#include "API/OperationRegistry.h"
#include "API/SchemaTemplate.h"

#include "Base/FrameBuffer.h"
#include "Data/ApplicationState.h"
#include "Exporters/Serializer.h"
#include "Misc/ViewportManager.h"
#include "Renderer/RendererViewport.h"

#include <avir/avir.h>
#include <stb/stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tf3d::api
{

    namespace
    {
        constexpr int JpegQuality = 90;

        bool ReadPositiveInt(const nlohmann::json &value, int &result)
        {
            if (value.is_number_unsigned()) {
                const std::uint64_t number = value.get<std::uint64_t>();
                if (number == 0 || number > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
                    return false;
                }
                result = static_cast<int>(number);
                return true;
            }

            if (!value.is_number_integer()) {
                return false;
            }
            const std::int64_t number = value.get<std::int64_t>();
            if (number <= 0 || number > std::numeric_limits<int>::max()) {
                return false;
            }
            result = static_cast<int>(number);
            return true;
        }

        void AppendJpegData(void *context, void *data, int size)
        {
            auto &jpeg        = *static_cast<std::vector<std::uint8_t> *>(context);
            const auto *bytes = static_cast<const std::uint8_t *>(data);
            jpeg.insert(jpeg.end(), bytes, bytes + size);
        }

        misc::ViewportManager *FindFirstViewportWithRenderer(tf3d::data::ApplicationState *applicationState)
        {
            if (applicationState == nullptr) {
                return nullptr;
            }

            for (misc::ViewportManager *viewport : applicationState->viewportManagers) {
                if (viewport != nullptr && viewport->GetRendererViewport() != nullptr) {
                    return viewport;
                }
            }
            return nullptr;
        }

        bool ResolveViewport(tf3d::data::ApplicationState *applicationState,
                             const nlohmann::json &arguments,
                             misc::ViewportManager *&viewport,
                             ApiResult &failure)
        {
            viewport = nullptr;
            if (applicationState == nullptr) {
                failure = ApiResult::Failure(
                    ApiErrorType::ViewportNotFound,
                    "TerraForge3D viewport state is unavailable.");
                return false;
            }

            if (!arguments.contains("ViewportId")) {
                failure = ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "'ViewportId' is required.");
                return false;
            }

            const auto &id  = arguments.at("ViewportId");
            int requestedId = 0;
            if (!ReadPositiveInt(id, requestedId)) {
                failure = ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "'ViewportId' must be a positive integer.");
                return false;
            }

            for (misc::ViewportManager *candidate : applicationState->viewportManagers) {
                if (candidate != nullptr && static_cast<int>(candidate->GetID()) == requestedId) {
                    viewport = candidate;
                    return true;
                }
            }

            failure = ApiResult::Failure(
                ApiErrorType::ViewportNotFound,
                "The requested viewport does not exist.");
            return false;
        }

        ApiResult UpdateViewportState(misc::ViewportManager &manager,
                                      const nlohmann::json &state,
                                      const nlohmann::json &readOnlySchema)
        {
            if (!state.is_object()) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "'State' must be an object.");
            }

            if (manager.GetRendererViewport() == nullptr) {
                return ApiResult::Failure(ApiErrorType::ViewportNotFound,
                                          "The requested viewport has no renderer state.");
            }

            if (state.contains("Mode")) {
                if (!state.at("Mode").is_string()) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                              "'Mode' must be a string.");
                }
                renderer::RendererViewportMode mode;
                if (!renderer::TryParseRendererViewportMode(state.at("Mode").get<std::string>(), mode)) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                              "'Mode' is not a supported viewport mode.");
                }
            }

            if (const auto cameraState = state.find("Camera");
                cameraState != state.end() && !cameraState->is_object()) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "'Camera' state must be an object.");
            }

            return operation_helpers::ApplySerializedState(
                state,
                &readOnlySchema,
                [&manager] { return manager.Save(); },
                [&manager](const auto &data) { manager.Load(data); });
        }

        ApiResult CaptureViewport(tf3d::data::ApplicationState *applicationState,
                                  const nlohmann::json &arguments)
        {
            int requestedMaxDimension = 0;
            if (arguments.contains("MaxDimension")) {
                const auto &maxDimension = arguments.at("MaxDimension");
                if (!ReadPositiveInt(maxDimension, requestedMaxDimension)) {
                    return ApiResult::Failure(
                        ApiErrorType::InvalidArguments,
                        "The optional 'MaxDimension' argument must be a positive integer.");
                }
            }

            misc::ViewportManager *viewport = nullptr;
            ApiResult failure;
            if (!ResolveViewport(applicationState, arguments, viewport, failure)) {
                return failure;
            }

            if (viewport->GetRendererViewport() == nullptr ||
                viewport->GetRendererViewport()->GetFrameBuffer() == nullptr) {
                return ApiResult::Failure(
                    ApiErrorType::ViewportNotFound,
                    "The requested viewport has no renderer framebuffer.");
            }

            const auto &frameBuffer = viewport->GetRendererViewport()->GetFrameBuffer();
            const int width         = frameBuffer->GetWidth();
            const int height        = frameBuffer->GetHeight();
            std::vector<std::uint8_t> pixels;
            if (!frameBuffer->DownloadColorToU8(pixels)) {
                return ApiResult::Failure(
                    ApiErrorType::ViewportReadFailed,
                    "TerraForge3D could not read the requested viewport framebuffer.");
            }

            int outputWidth  = width;
            int outputHeight = height;
            if (requestedMaxDimension > 0 && std::max(width, height) > requestedMaxDimension) {
                const double scale = static_cast<double>(requestedMaxDimension) /
                                     static_cast<double>(std::max(width, height));
                outputWidth  = std::max(1, static_cast<int>(std::lround(width * scale)));
                outputHeight = std::max(1, static_cast<int>(std::lround(height * scale)));

                std::vector<std::uint8_t> resized(
                    static_cast<std::size_t>(outputWidth) * static_cast<std::size_t>(outputHeight) * 3u);
                avir::CImageResizer<> imageResizer(8);
                imageResizer.resizeImage(
                    pixels.data(), width, height, 0,
                    resized.data(), outputWidth, outputHeight, 3, 0);
                pixels = std::move(resized);
            }

            std::vector<std::uint8_t> jpeg;
            if (stbi_write_jpg_to_func(
                    AppendJpegData,
                    &jpeg,
                    outputWidth,
                    outputHeight,
                    3,
                    pixels.data(),
                    JpegQuality) == 0 ||
                jpeg.empty()) {
                return ApiResult::Failure(
                    ApiErrorType::JpegEncodeFailed,
                    "TerraForge3D could not encode the requested viewport as JPEG.");
            }

            return ApiResult::SuccessWithBinary(
                {{"mimeType", "image/jpeg"},
                 {"Width", outputWidth},
                 {"Height", outputHeight}},
                "image/jpeg",
                std::move(jpeg));
        }

        ApiResult ListVisibleViewports(tf3d::data::ApplicationState *applicationState)
        {
            if (applicationState == nullptr) {
                return ApiResult::Failure(
                    ApiErrorType::ViewportNotFound,
                    "TerraForge3D viewport state is unavailable.");
            }

            nlohmann::json viewports = nlohmann::json::array();
            for (misc::ViewportManager *viewport : applicationState->viewportManagers) {
                if (viewport == nullptr || !viewport->IsVisible()) {
                    continue;
                }

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

            return ApiResult::Success(
                {{"Viewports", std::move(viewports)}});
        }

        ApiSchemaRuntimeProvider BuildViewportSchemaRuntime(tf3d::data::ApplicationState *applicationState)
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
                    for (int value = 0; value < 6; ++value) {
                        slots.push_back(value);
                    }
                    return slots;
                }

                if (name == "Viewport.TextureChannelIndices") {
                    nlohmann::json channels         = nlohmann::json::array();
                    misc::ViewportManager *viewport = FindFirstViewportWithRenderer(applicationState);
                    const int channelCount          = viewport == nullptr
                                                          ? 4
                                                          : static_cast<int>(viewport->GetRendererViewport()
                                                                                 ->GetTextureSlotDetailed()
                                                                                 .size());
                    for (int value = 0; value < channelCount; ++value) {
                        channels.push_back(value);
                    }
                    return channels;
                }

                if (name == "Viewport.TextureChannelCount") {
                    misc::ViewportManager *viewport = FindFirstViewportWithRenderer(applicationState);
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

    void RegisterViewportOperations(OperationRegistry &operations, tf3d::data::ApplicationState *applicationState)
    {
        const ApiSchemaRuntimeProvider runtime = BuildViewportSchemaRuntime(applicationState);
        const auto updateReadOnlySchema =
            ApiSchemaTemplate::ComposeDefault("Operations/Viewport/UpdateStateReadOnly.json");

        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Viewport/Definitions/List.json",
            [applicationState](const nlohmann::json &) {
                return ListVisibleViewports(applicationState);
            });

        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Viewport/Definitions/GetState.json",
            [applicationState](const nlohmann::json &arguments) {
                misc::ViewportManager *viewport = nullptr;
                ApiResult failure;
                if (!ResolveViewport(applicationState, arguments, viewport, failure)) {
                    return failure;
                }
                if (viewport->GetRendererViewport() == nullptr) {
                    return ApiResult::Failure(
                        ApiErrorType::ViewportNotFound,
                        "The requested viewport has no renderer state.");
                }
                return operation_helpers::GetSerializedState(
                    [viewport] { return viewport->Save(); });
            });

        if (updateReadOnlySchema) {
            TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(
                operations,
                "Operations/Viewport/Definitions/UpdateState.json",
                runtime,
                [applicationState, readOnlySchema = *updateReadOnlySchema](
                    const nlohmann::json &arguments) {
                    if (!arguments.contains("State")) {
                        return ApiResult::Failure(
                            ApiErrorType::InvalidArguments,
                            "'State' is required.");
                    }
                    misc::ViewportManager *viewport = nullptr;
                    ApiResult failure;
                    if (!ResolveViewport(applicationState, arguments, viewport, failure)) {
                        return failure;
                    }
                    return UpdateViewportState(*viewport, arguments.at("State"), readOnlySchema);
                });
        }

        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Viewport/Definitions/Capture.json",
            [applicationState](const nlohmann::json &arguments) {
                return CaptureViewport(applicationState, arguments);
            });
    }

} // namespace tf3d::api
