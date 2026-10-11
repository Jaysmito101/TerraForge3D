#include "API/Operations/MaskOperations.h"

#include "API/ApiResult.h"
#include "API/OperationRegistry.h"
#include "API/SchemaTemplate.h"
#include "Data/ApplicationState.h"
#include "Exporters/Serializer.h"
#include "Generators/BaseNoiseGenerator.h"
#include "Generators/BiomeCustomBaseShape.h"
#include "Generators/BiomeManager.h"
#include "Generators/Filters/BiomeFilter.h"
#include "Generators/GenerationManager.h"
#include "Generators/Masks/MaskLayer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tf3d::api
{

    namespace
    {
        using exporters::CreateSerializerNodeFromJson;
        using generators::BiomeManager;
        using generators::MaskLayer;
        using generators::MaskStroke;
        using generators::MaskStrokeMode;

        constexpr std::size_t MaxStrokeCount       = 128;
        constexpr std::size_t MaxPointsPerStroke   = 4096;
        constexpr std::size_t MaxPointsPerAppend   = 1024;
        constexpr std::size_t MaxTotalStrokePoints = 65536;
        constexpr float MinBrushSize               = 0.005f;
        constexpr float MaxBrushSize               = 2.0f;

        enum class MaskKind {
            Biome,
            BaseNoise,
            CustomBaseShape,
            Filter
        };

        struct MaskTarget {
            MaskKind kind = MaskKind::Biome;
            std::string filterId;
            std::size_t maskIndex = 0;
        };

        struct ResolvedMask {
            MaskLayer *layer = nullptr;
            std::function<bool()> publishOwnerState;
        };

        std::string_view MaskKindName(MaskKind kind)
        {
            switch (kind) {
                case MaskKind::Biome:
                    return "biome";
                case MaskKind::BaseNoise:
                    return "base-noise";
                case MaskKind::CustomBaseShape:
                    return "custom-base-shape";
                case MaskKind::Filter:
                    return "filter";
            }
            return "biome";
        }

        bool ParseMaskKind(std::string_view name, MaskKind &kind)
        {
            if (name == "biome") {
                kind = MaskKind::Biome;
                return true;
            }
            if (name == "base-noise") {
                kind = MaskKind::BaseNoise;
                return true;
            }
            if (name == "custom-base-shape") {
                kind = MaskKind::CustomBaseShape;
                return true;
            }
            if (name == "filter") {
                kind = MaskKind::Filter;
                return true;
            }
            return false;
        }

        ApiResult ResolveBiome(tf3d::data::ApplicationState *applicationState,
                               const nlohmann::json &arguments,
                               BiomeManager *&biome)
        {
            biome = nullptr;
            if (applicationState == nullptr || applicationState->generationManager == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }
            const std::string biomeId = arguments.value("BiomeId", std::string{});
            if (biomeId.empty()) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, "BiomeId is required.");
            }
            biome = applicationState->generationManager->FindBiome(biomeId);
            if (biome == nullptr) {
                return ApiResult::Failure(ApiErrorType::BiomeNotFound,
                                          "BiomeId does not exist in the generation graph.");
            }
            return ApiResult::Success();
        }

        bool ReadTarget(const nlohmann::json &arguments,
                        MaskTarget &target,
                        ApiResult &failure)
        {
            const std::string kindName = arguments.value("MaskKind", std::string{});
            if (!ParseMaskKind(kindName, target.kind)) {
                failure = ApiResult::Failure(
                    ApiErrorType::InvalidArguments,
                    "MaskKind must be biome, base-noise, custom-base-shape, or filter.");
                return false;
            }

            const bool hasFilterId  = arguments.contains("FilterId");
            const bool hasMaskIndex = arguments.contains("MaskIndex");
            switch (target.kind) {
                case MaskKind::Biome:
                case MaskKind::BaseNoise:
                    if (hasFilterId || hasMaskIndex) {
                        failure = ApiResult::Failure(
                            ApiErrorType::InvalidArguments,
                            "FilterId and MaskIndex do not apply to this MaskKind.");
                        return false;
                    }
                    return true;
                case MaskKind::Filter:
                    if (!hasFilterId || hasMaskIndex || !arguments.at("FilterId").is_string() ||
                        arguments.at("FilterId").get_ref<const std::string &>().empty()) {
                        failure = ApiResult::Failure(
                            ApiErrorType::InvalidArguments,
                            "Filter masks require FilterId and do not accept MaskIndex.");
                        return false;
                    }
                    target.filterId = arguments.at("FilterId").get<std::string>();
                    return true;
                case MaskKind::CustomBaseShape:
                    if (!hasMaskIndex || hasFilterId || !arguments.at("MaskIndex").is_number_integer()) {
                        failure = ApiResult::Failure(
                            ApiErrorType::InvalidArguments,
                            "Custom base-shape masks require a non-negative MaskIndex and do not accept FilterId.");
                        return false;
                    }
                    if (arguments.at("MaskIndex").get<std::int64_t>() < 0) {
                        failure = ApiResult::Failure(ApiErrorType::InvalidArguments,
                                                     "MaskIndex must be non-negative.");
                        return false;
                    }
                    target.maskIndex = static_cast<std::size_t>(
                        arguments.at("MaskIndex").get<std::uint64_t>());
                    return true;
            }
            return false;
        }

        ApiResult ResolveMask(BiomeManager &biome,
                              const MaskTarget &target,
                              ResolvedMask &resolved)
        {
            resolved = {};
            switch (target.kind) {
                case MaskKind::Biome:
                    resolved.layer             = biome.GetMaskLayer();
                    resolved.publishOwnerState = [] { return true; };
                    break;
                case MaskKind::BaseNoise: {
                    auto *baseNoise = biome.GetBaseNoiseGenerator();
                    if (baseNoise != nullptr) {
                        resolved.layer             = baseNoise->GetMaskLayer();
                        resolved.publishOwnerState = [baseNoise] {
                            baseNoise->PublishMaskState();
                            return true;
                        };
                    }
                    break;
                }
                case MaskKind::CustomBaseShape: {
                    auto *customShape = biome.GetCustomBaseShape();
                    if (customShape != nullptr) {
                        resolved.layer             = customShape->GetMaskLayer(target.maskIndex);
                        resolved.publishOwnerState = [customShape] {
                            customShape->PublishMaskState();
                            return true;
                        };
                    }
                    break;
                }
                case MaskKind::Filter: {
                    const auto filter = biome.FindFilter(target.filterId);
                    if (filter == nullptr) {
                        return ApiResult::Failure(ApiErrorType::FilterNotFound,
                                                  "FilterId does not exist in this biome.");
                    }
                    resolved.layer             = filter->GetMaskLayer();
                    BiomeManager *biomeManager = &biome;
                    resolved.publishOwnerState = [biomeManager, filterId = target.filterId] {
                        return biomeManager->PublishFilterMaskState(filterId);
                    };
                    break;
                }
            }
            if (resolved.layer == nullptr || !resolved.publishOwnerState) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "The selected mask target is unavailable.");
            }
            return ApiResult::Success();
        }

        nlohmann::json BuildMaskReference(std::string_view operationId,
                                          const std::string &biomeId,
                                          const MaskTarget &target)
        {
            nlohmann::json parameters = {
                {"BiomeId", biomeId},
                {"MaskKind", std::string(MaskKindName(target.kind))}};
            if (target.kind == MaskKind::Filter) {
                parameters["FilterId"] = target.filterId;
            } else if (target.kind == MaskKind::CustomBaseShape) {
                parameters["MaskIndex"] = std::to_string(target.maskIndex);
            }
            return {{"operationId", operationId}, {"parameters", std::move(parameters)}};
        }

        nlohmann::json BuildMaskState(const std::string &biomeId,
                                      const MaskTarget &target,
                                      const MaskLayer &layer)
        {
            const auto snapshot     = layer.GetState();
            const auto &tool        = snapshot.value.tool;
            std::size_t totalPoints = 0;
            nlohmann::json strokes  = nlohmann::json::array();
            for (std::size_t index = 0; index < tool.strokes.size(); ++index) {
                const std::size_t pointCount = tool.strokes[index].points.size();
                totalPoints += pointCount;
                strokes.push_back({{"index", index}, {"pointCount", pointCount}});
            }

            nlohmann::json activeStroke = nullptr;
            if (tool.activeStroke.has_value()) {
                const std::size_t pointCount = tool.activeStroke->points.size();
                totalPoints += pointCount;
                activeStroke = {{"pointCount", pointCount}};
            }

            nlohmann::json allowedModes = {"paint", "erase"};
            if (layer.AllowsNegativeValues()) {
                allowedModes.push_back("paint-negative");
            }

            nlohmann::json limits = {
                {"maxCompletedStrokes", MaxStrokeCount},
                {"maxPointsPerStroke", MaxPointsPerStroke},
                {"maxPointsPerAppend", MaxPointsPerAppend},
                {"maxTotalPoints", MaxTotalStrokePoints},
                {"brushSize", {{"minimum", MinBrushSize}, {"maximum", MaxBrushSize}}},
                {"strength", {{"minimum", 0.0}, {"maximum", 1.0}}},
                {"falloff", {{"minimum", 0.0}, {"maximum", 1.0}}}};
            nlohmann::json operations = nlohmann::json::object();
            operations["update"]      = BuildMaskReference(
                "tf3d.generation.biomes.masks.update", biomeId, target);
            operations["beginStroke"] = BuildMaskReference(
                "tf3d.generation.biomes.masks.strokes.begin", biomeId, target);
            operations["appendStroke"] = BuildMaskReference(
                "tf3d.generation.biomes.masks.strokes.append", biomeId, target);
            operations["finishStroke"] = BuildMaskReference(
                "tf3d.generation.biomes.masks.strokes.finish", biomeId, target);
            operations["undoStroke"] = BuildMaskReference(
                "tf3d.generation.biomes.masks.strokes.undo", biomeId, target);
            operations["clearStrokes"] = BuildMaskReference(
                "tf3d.generation.biomes.masks.strokes.clear", biomeId, target);

            nlohmann::json response = {
                {"reference", BuildMaskReference("tf3d.generation.biomes.masks.get", biomeId, target)},
                {"kind", std::string(MaskKindName(target.kind))},
                {"revision", std::to_string(layer.GetStateRevision())},
                {"processedRevision", std::to_string(layer.GetProcessedRevision())},
                {"dirty", layer.RequireUpdation()},
                {"resolution", layer.GetResolution()},
                {"baseMask", layer.SaveBaseMaskSettings()->ToJson()},
                {"baseMaskSchema", BuildMaskReference("tf3d.generation.biomes.masks.get_schema", biomeId, target)},
                {"preview", {{"visible", layer.GetPreviewVisible()}, {"inverted", layer.GetInvertPreview()}}},
                {"strokes", std::move(strokes)},
                {"activeStroke", std::move(activeStroke)},
                {"strokeCount", tool.strokes.size()},
                {"pointCount", totalPoints},
                {"allowedModes", std::move(allowedModes)}};
            nlohmann::json coordinateSpace = nlohmann::json::object();
            coordinateSpace["type"]        = "normalized-terrain-uv";
            coordinateSpace["minimum"]     = 0.0;
            coordinateSpace["maximum"]     = 1.0;
            coordinateSpace["uAxis"]       = "0 at minimum world X; 1 at maximum world X";
            coordinateSpace["vAxis"]       = "0 at maximum world Z; 1 at minimum world Z";
            coordinateSpace["resolutionMapping"] =
                "Texel centers are (x + 0.5) / resolution, (y + 0.5) / resolution.";
            response["coordinateSpace"] = std::move(coordinateSpace);
            response["limits"]          = std::move(limits);
            response["operations"]      = std::move(operations);
            return response;
        }

        ApiResult ResolveMaskArguments(tf3d::data::ApplicationState *applicationState,
                                       const nlohmann::json &arguments,
                                       BiomeManager *&biome,
                                       MaskTarget &target,
                                       ResolvedMask &resolved)
        {
            ApiResult failure = ResolveBiome(applicationState, arguments, biome);
            if (!failure.IsSuccess()) {
                return failure;
            }
            if (!ReadTarget(arguments, target, failure)) {
                return failure;
            }
            return ResolveMask(*biome, target, resolved);
        }

        ApiResult GetMask(tf3d::data::ApplicationState *applicationState,
                          const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            MaskTarget target;
            ResolvedMask resolved;
            ApiResult result = ResolveMaskArguments(applicationState, arguments, biome, target, resolved);
            if (!result.IsSuccess()) {
                return result;
            }
            return ApiResult::Success(BuildMaskState(arguments.at("BiomeId").get<std::string>(),
                                                     target,
                                                     *resolved.layer));
        }

        ApiResult GetMaskSchema(tf3d::data::ApplicationState *applicationState,
                                const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            MaskTarget target;
            ResolvedMask resolved;
            ApiResult result = ResolveMaskArguments(applicationState, arguments, biome, target, resolved);
            if (!result.IsSuccess()) {
                return result;
            }
            return ApiResult::Success({{"kind", std::string(MaskKindName(target.kind))},
                                       {"baseMask", resolved.layer->GetBaseMaskSchema()}});
        }

        ApiResult UpdateMask(tf3d::data::ApplicationState *applicationState,
                             const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            MaskTarget target;
            ResolvedMask resolved;
            ApiResult result = ResolveMaskArguments(applicationState, arguments, biome, target, resolved);
            if (!result.IsSuccess()) {
                return result;
            }

            const nlohmann::json &state = arguments.at("State");
            const bool updatesBase      = state.contains("BaseMask");
            const bool updatesPreview   = state.contains("Preview");
            if (!updatesBase && !updatesPreview) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "State must update BaseMask or Preview.");
            }
            if (updatesBase && state.at("BaseMask").empty()) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "BaseMask must contain at least one setting.");
            }
            if (updatesPreview && state.at("Preview").empty()) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "Preview must update Visible or Inverted.");
            }

            if (updatesBase) {
                const nlohmann::json &baseMaskPatch = state.at("BaseMask");
                nlohmann::json validationErrors     = nlohmann::json::array();
                const nlohmann::json baseMaskSchema = resolved.layer->GetBaseMaskSchema();
                if (!ApiSchemaTemplate::Validate(baseMaskPatch, baseMaskSchema, validationErrors)) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                              "BaseMask does not match this mask layer's schema.",
                                              {{"errors", std::move(validationErrors)}});
                }
                std::string readOnlyError;
                if (!ApiSchemaTemplate::ValidateWritable(baseMaskPatch, baseMaskSchema, readOnlyError)) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(readOnlyError));
                }
                std::string error;
                if (!resolved.layer->UpdateBaseMaskSettings(
                        CreateSerializerNodeFromJson(baseMaskPatch), error)) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
                }
                if (!resolved.publishOwnerState()) {
                    return ApiResult::Failure(ApiErrorType::TaskException,
                                              "The owning generation node could not publish the updated mask.");
                }
                biome->MarkMaskUpdated();
            }

            if (updatesPreview) {
                const nlohmann::json &preview = state.at("Preview");
                resolved.layer->SetPreviewSettings(
                    preview.value("Visible", resolved.layer->GetPreviewVisible()),
                    preview.value("Inverted", resolved.layer->GetInvertPreview()));
            }

            return ApiResult::Success(BuildMaskState(arguments.at("BiomeId").get<std::string>(),
                                                     target,
                                                     *resolved.layer));
        }

        bool ReadPoint(const nlohmann::json &value,
                       glm::vec2 &point,
                       std::string &error)
        {
            if (!value.is_object() || !value.contains("U") || !value.contains("V") ||
                !value.at("U").is_number() || !value.at("V").is_number()) {
                error = "Each point must contain numeric U and V values.";
                return false;
            }
            point = glm::vec2(value.at("U").get<float>(), value.at("V").get<float>());
            if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
                point.x < 0.0f || point.x > 1.0f || point.y < 0.0f || point.y > 1.0f) {
                error = "Point U and V coordinates must be finite and within [0, 1].";
                return false;
            }
            return true;
        }

        std::size_t CountPoints(const generators::MaskTool::State &state)
        {
            std::size_t count = 0;
            for (const auto &stroke : state.strokes) {
                count += stroke.points.size();
            }
            if (state.activeStroke.has_value()) {
                count += state.activeStroke->points.size();
            }
            return count;
        }

        bool ParseStrokeMode(const nlohmann::json &value,
                             MaskLayer &layer,
                             MaskStrokeMode &mode,
                             std::string &error)
        {
            if (!value.is_string()) {
                error = "Mode must be paint, erase, or paint-negative.";
                return false;
            }
            const std::string name = value.get<std::string>();
            if (name == "paint") {
                mode = MaskStrokeMode::Paint;
                return true;
            }
            if (name == "erase") {
                mode = MaskStrokeMode::Erase;
                return true;
            }
            if (name == "paint-negative" && layer.AllowsNegativeValues()) {
                mode = MaskStrokeMode::PaintNegative;
                return true;
            }
            error = layer.AllowsNegativeValues()
                        ? "Mode must be paint, erase, or paint-negative."
                        : "Mode must be paint or erase for this mask layer.";
            return false;
        }

        ApiResult MutationResult(BiomeManager &biome,
                                 const std::string &biomeId,
                                 const MaskTarget &target,
                                 const ResolvedMask &resolved,
                                 bool changed)
        {
            if (changed) {
                if (!resolved.publishOwnerState()) {
                    return ApiResult::Failure(ApiErrorType::TaskException,
                                              "The owning generation node could not publish the updated mask.");
                }
                biome.MarkMaskUpdated();
            }
            return ApiResult::Success(BuildMaskState(biomeId, target, *resolved.layer));
        }

        ApiResult BeginStroke(tf3d::data::ApplicationState *applicationState,
                              const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            MaskTarget target;
            ResolvedMask resolved;
            ApiResult result = ResolveMaskArguments(applicationState, arguments, biome, target, resolved);
            if (!result.IsSuccess()) {
                return result;
            }

            const auto maskState  = resolved.layer->GetState();
            const auto &state     = maskState.value.tool;
            const auto pointCount = CountPoints(state);
            if (state.activeStroke.has_value()) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "A stroke is already active for this mask.");
            }
            if (state.strokes.size() >= MaxStrokeCount || pointCount >= MaxTotalStrokePoints) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "The mask has reached its retained stroke or point limit; clear or undo strokes first.");
            }

            MaskStroke stroke;
            glm::vec2 point;
            std::string pointError;
            if (!ReadPoint(arguments.at("Point"), point, pointError)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(pointError));
            }
            stroke.points.push_back(point);
            stroke.size     = arguments.at("Size").get<float>();
            stroke.strength = arguments.at("Strength").get<float>();
            stroke.falloff  = arguments.at("Falloff").get<float>();
            std::string modeError;
            if (!ParseStrokeMode(arguments.at("Mode"), *resolved.layer, stroke.mode, modeError)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(modeError));
            }
            if (!std::isfinite(stroke.size) || stroke.size < MinBrushSize || stroke.size > MaxBrushSize ||
                !std::isfinite(stroke.strength) || stroke.strength < 0.0f || stroke.strength > 1.0f ||
                !std::isfinite(stroke.falloff) || stroke.falloff < 0.0f || stroke.falloff > 1.0f) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "Size, Strength, and Falloff must be finite and within their documented limits.");
            }
            if (!resolved.layer->BeginStroke(std::move(stroke))) {
                return ApiResult::Failure(ApiErrorType::TaskException,
                                          "The mask could not begin the stroke.");
            }
            return MutationResult(*biome,
                                  arguments.at("BiomeId").get<std::string>(),
                                  target,
                                  resolved,
                                  true);
        }

        ApiResult AppendStroke(tf3d::data::ApplicationState *applicationState,
                               const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            MaskTarget target;
            ResolvedMask resolved;
            ApiResult result = ResolveMaskArguments(applicationState, arguments, biome, target, resolved);
            if (!result.IsSuccess()) {
                return result;
            }

            const auto maskState = resolved.layer->GetState();
            const auto &state    = maskState.value.tool;
            if (!state.activeStroke.has_value()) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "Begin a stroke before appending points.");
            }
            const auto &pointsValue = arguments.at("Points");
            if (pointsValue.empty() || pointsValue.size() > MaxPointsPerAppend) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "Points must contain between 1 and 1024 points per append request.");
            }
            if (state.activeStroke->points.size() + pointsValue.size() > MaxPointsPerStroke ||
                CountPoints(state) + pointsValue.size() > MaxTotalStrokePoints) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "Appending these points would exceed the stroke or total point limit.");
            }

            std::vector<glm::vec2> points;
            points.reserve(pointsValue.size());
            for (const auto &value : pointsValue) {
                glm::vec2 point;
                std::string error;
                if (!ReadPoint(value, point, error)) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
                }
                points.push_back(point);
            }

            if (!resolved.layer->AppendStrokePoints(points)) {
                return ApiResult::Failure(ApiErrorType::TaskException,
                                          "The mask could not append stroke points.");
            }
            return MutationResult(*biome,
                                  arguments.at("BiomeId").get<std::string>(),
                                  target,
                                  resolved,
                                  true);
        }

        ApiResult FinishStroke(tf3d::data::ApplicationState *applicationState,
                               const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            MaskTarget target;
            ResolvedMask resolved;
            ApiResult result = ResolveMaskArguments(applicationState, arguments, biome, target, resolved);
            if (!result.IsSuccess()) {
                return result;
            }

            const auto maskState = resolved.layer->GetState();
            const auto &state    = maskState.value.tool;
            if (!state.activeStroke.has_value()) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "There is no active stroke to finish.");
            }
            if (state.activeStroke->points.empty() ||
                state.activeStroke->points.size() > MaxPointsPerStroke ||
                CountPoints(state) > MaxTotalStrokePoints || state.strokes.size() >= MaxStrokeCount) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "The active stroke cannot be finished because a retention limit is exceeded.");
            }
            if (!resolved.layer->FinishStroke()) {
                return ApiResult::Failure(ApiErrorType::TaskException,
                                          "The active mask stroke could not be finished.");
            }
            return MutationResult(*biome,
                                  arguments.at("BiomeId").get<std::string>(),
                                  target,
                                  resolved,
                                  true);
        }

        ApiResult UndoStroke(tf3d::data::ApplicationState *applicationState,
                             const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            MaskTarget target;
            ResolvedMask resolved;
            ApiResult result = ResolveMaskArguments(applicationState, arguments, biome, target, resolved);
            if (!result.IsSuccess()) {
                return result;
            }
            const bool changed = resolved.layer->UndoStroke();
            return MutationResult(*biome,
                                  arguments.at("BiomeId").get<std::string>(),
                                  target,
                                  resolved,
                                  changed);
        }

        ApiResult ClearStrokes(tf3d::data::ApplicationState *applicationState,
                               const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            MaskTarget target;
            ResolvedMask resolved;
            ApiResult result = ResolveMaskArguments(applicationState, arguments, biome, target, resolved);
            if (!result.IsSuccess()) {
                return result;
            }
            const bool changed = resolved.layer->ClearStrokes();
            return MutationResult(*biome,
                                  arguments.at("BiomeId").get<std::string>(),
                                  target,
                                  resolved,
                                  changed);
        }
    } // namespace

    void RegisterMaskOperations(OperationRegistry &operations,
                                tf3d::data::ApplicationState *applicationState)
    {
        const ApiSchemaRuntimeProvider runtime = [applicationState](std::string_view name)
            -> std::optional<nlohmann::json> {
            if (name != "Generation.BaseMask" || applicationState == nullptr ||
                applicationState->generationManager == nullptr) {
                return std::nullopt;
            }
            const auto &biomes = applicationState->generationManager->GetBiomes();
            const auto found   = std::find_if(biomes.begin(), biomes.end(), [](const auto &biome) {
                return biome != nullptr;
            });
            return found != biomes.end()
                       ? std::optional<nlohmann::json>((*found)->GetBaseMaskSchema())
                       : std::optional<nlohmann::json>(nlohmann::json{
                             {"type", "object"}, {"additionalProperties", false}});
        };
        const auto registerMaskOperation = [&operations, &runtime](
                                               std::string_view schemaPath,
                                               ApiOperationHandler handler) {
            RegisterOperationFromJson(
                operations,
                ApiSchemaTemplate::ComposeDefault(schemaPath, runtime),
                std::move(handler));
        };

        registerMaskOperation(
            "Operations/Generation/Masks/Definitions/Get.json",
            [applicationState](const nlohmann::json &arguments) {
                return GetMask(applicationState, arguments);
            });
        registerMaskOperation(
            "Operations/Generation/Masks/Definitions/GetSchema.json",
            [applicationState](const nlohmann::json &arguments) {
                return GetMaskSchema(applicationState, arguments);
            });
        registerMaskOperation(
            "Operations/Generation/Masks/Definitions/Update.json",
            [applicationState](const nlohmann::json &arguments) {
                return UpdateMask(applicationState, arguments);
            });
        registerMaskOperation(
            "Operations/Generation/Masks/Definitions/BeginStroke.json",
            [applicationState](const nlohmann::json &arguments) {
                return BeginStroke(applicationState, arguments);
            });
        registerMaskOperation(
            "Operations/Generation/Masks/Definitions/AppendStroke.json",
            [applicationState](const nlohmann::json &arguments) {
                return AppendStroke(applicationState, arguments);
            });
        registerMaskOperation(
            "Operations/Generation/Masks/Definitions/FinishStroke.json",
            [applicationState](const nlohmann::json &arguments) {
                return FinishStroke(applicationState, arguments);
            });
        registerMaskOperation(
            "Operations/Generation/Masks/Definitions/UndoStroke.json",
            [applicationState](const nlohmann::json &arguments) {
                return UndoStroke(applicationState, arguments);
            });
        registerMaskOperation(
            "Operations/Generation/Masks/Definitions/ClearStrokes.json",
            [applicationState](const nlohmann::json &arguments) {
                return ClearStrokes(applicationState, arguments);
            });
    }

} // namespace tf3d::api
