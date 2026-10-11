#include "API/Operations/GenerationGraphOperations.h"

#include "API/ApiResult.h"
#include "API/OperationHelpers.h"
#include "API/OperationRegistry.h"
#include "API/SchemaTemplate.h"
#include "Data/ApplicationState.h"
#include "Exporters/Serializer.h"
#include "Generators/BiomeManager.h"
#include "Generators/Filters/BiomeFilter.h"
#include "Generators/Filters/BiomeFilterDefinition.h"
#include "Generators/GenerationManager.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tf3d::api
{

    namespace
    {
        using exporters::CreateSerializerNodeFromJson;
        using exporters::SerializerNode;
        using generators::BiomeFilter;
        using generators::BiomeFilterDefinition;
        using generators::BiomeManager;
        using generators::GenerationManager;

        GenerationManager *GetGenerationManager(tf3d::data::ApplicationState *applicationState)
        {
            return applicationState != nullptr ? applicationState->generationManager : nullptr;
        }

        bool ResolveBiome(tf3d::data::ApplicationState *applicationState,
                          const nlohmann::json &arguments,
                          BiomeManager *&biome,
                          ApiResult &failure)
        {
            biome                         = nullptr;
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                failure = ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
                return false;
            }
            const auto id = arguments.find("BiomeId");
            if (id == arguments.end() || !id->is_string()) {
                failure = ApiResult::Failure(ApiErrorType::InvalidArguments, "BiomeId is required.");
                return false;
            }
            biome = generation->FindBiome(id->get<std::string>());
            if (biome == nullptr) {
                failure = ApiResult::Failure(ApiErrorType::BiomeNotFound,
                                             "BiomeId does not exist in the generation graph.");
                return false;
            }
            return true;
        }

        std::string MergeModeName(int32_t mode)
        {
            static constexpr std::string_view names[] = {"override", "add", "subtract", "multiply", "blend"};
            return mode >= 0 && mode < static_cast<int32_t>(std::size(names))
                       ? std::string(names[mode])
                       : std::string("blend");
        }

        bool ParseMergeMode(std::string_view name, int32_t &mode)
        {
            static constexpr std::string_view names[] = {"override", "add", "subtract", "multiply", "blend"};
            for (int32_t index = 0; index < static_cast<int32_t>(std::size(names)); ++index) {
                if (names[index] == name) {
                    mode = index;
                    return true;
                }
            }
            return false;
        }

        nlohmann::json BuildMaskOperationReference(std::string_view operationId,
                                                   std::string_view biomeId,
                                                   std::string_view kind,
                                                   std::string_view targetKey   = {},
                                                   std::string_view targetValue = {})
        {
            nlohmann::json parameters = {
                {"BiomeId", std::string(biomeId)},
                {"MaskKind", std::string(kind)}};
            if (!targetKey.empty()) {
                parameters[std::string(targetKey)] = std::string(targetValue);
            }
            return {{"operationId", operationId}, {"parameters", std::move(parameters)}};
        }

        nlohmann::json BuildMaskReference(std::string_view biomeId,
                                          std::string_view kind,
                                          std::string_view targetKey   = {},
                                          std::string_view targetValue = {})
        {
            return BuildMaskOperationReference("tf3d.generation.biomes.masks.get",
                                               biomeId,
                                               kind,
                                               targetKey,
                                               targetValue);
        }

        nlohmann::json BuildFilterState(const BiomeFilter &filter,
                                        int32_t position,
                                        std::string_view biomeId)
        {
            nlohmann::json state = filter.Save()->ToJson();
            state.erase("BaseMask");
            state.erase("MaskTool");
            state["MergeMode"] = MergeModeName(state.value("MergeMode", 4));
            state["Name"]      = filter.GetName();
            state["Category"]  = filter.GetCategory();
            state["Position"]  = position;
            state["Mask"]      = BuildMaskReference(biomeId,
                                                    "filter",
                                                    "FilterId",
                                                    filter.GetID());
            state["Schema"]    = {
                {"operationId", "tf3d.generation.filter_catalog.get_schema"},
                {"parameters", {{"FilterDefinitionId", filter.GetDefinitionID()}}}};
            return state;
        }

        int32_t FindFilterPosition(const BiomeManager &biome, std::string_view filterId)
        {
            const auto &filters = biome.GetFilters();
            const auto found    = std::find_if(filters.begin(), filters.end(), [filterId](const auto &filter) {
                return filter != nullptr && filter->GetID() == filterId;
            });
            return found == filters.end() ? -1 : static_cast<int32_t>(std::distance(filters.begin(), found));
        }

        std::shared_ptr<BiomeFilterDefinition> FindFilterDefinition(
            const GenerationManager &generation,
            std::string_view definitionId)
        {
            for (const auto &biome : generation.GetBiomes()) {
                if (biome == nullptr) {
                    continue;
                }
                const auto &definitions = biome->GetFilterDefinitions();
                const auto found        = std::find_if(definitions.begin(), definitions.end(), [definitionId](const auto &item) {
                    return item != nullptr && item->GetID() == definitionId;
                });
                if (found != definitions.end()) {
                    return *found;
                }
            }
            return nullptr;
        }

        std::string Lower(std::string_view value)
        {
            std::string result(value);
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
            return result;
        }

        nlohmann::json BuildBaseShapeAlgorithmSchemaLink(std::string_view algorithmId)
        {
            return {
                {"operationId", "tf3d.generation.base_shape_algorithms.get_schema"},
                {"parameters", {{"AlgorithmId", algorithmId}}}};
        }

        nlohmann::json BuildBiomeSummary(BiomeManager &biome)
        {
            const SerializerNode baseShape   = biome.SaveBaseShapeSettings();
            const SerializerNode baseNoise   = biome.SaveBaseNoiseSettings();
            const SerializerNode customShape = biome.SaveCustomBaseShapeSettings();
            return {
                {"filterCount", biome.GetFiltersCount()},
                {"baseShapeMode", baseShape->Get<std::string>("Mode", "algorithm")},
                {"baseShapeAlgorithmId", baseShape->Get<std::string>("AlgorithmID", "")},
                {"baseNoiseEnabled", baseNoise->Get<bool>("Enabled", false)},
                {"customBaseShapeEnabled", customShape->Get<bool>("Enabled", false)}};
        }

        nlohmann::json BuildBiomeDirectory(const GenerationManager &generation)
        {
            nlohmann::json biomes = nlohmann::json::array();
            const auto &entries   = generation.GetBiomes();
            for (std::size_t index = 0; index < entries.size(); ++index) {
                const auto &biome = entries[index];
                if (biome == nullptr) {
                    continue;
                }
                biomes.push_back({{"id", biome->GetBiomeID().GetValue()},
                                  {"order", index},
                                  {"name", biome->GetBiomeName()},
                                  {"enabled", biome->IsEnabled()},
                                  {"summary", BuildBiomeSummary(*biome)}});
            }
            return {{"items", std::move(biomes)}, {"count", entries.size()}, {"minimumCount", 1}, {"maximumCount", 64}};
        }

        nlohmann::json BuildBaseShapeResponse(BiomeManager &biome)
        {
            nlohmann::json response   = biome.SaveBaseShapeSettings()->ToJson();
            nlohmann::json algorithms = nlohmann::json::array();
            for (const auto &generator : biome.GetBaseShapeGenerators()) {
                if (generator != nullptr) {
                    algorithms.push_back({{"id", generator->GetID()},
                                          {"name", generator->GetName()},
                                          {"description", generator->GetDescription()},
                                          {"schema", BuildBaseShapeAlgorithmSchemaLink(generator->GetID())}});
                }
            }
            response["Algorithms"]      = std::move(algorithms);
            response["AlgorithmSchema"] = BuildBaseShapeAlgorithmSchemaLink(response.value("AlgorithmID", ""));
            return response;
        }

        nlohmann::json BuildBaseNoiseResponse(BiomeManager &biome)
        {
            nlohmann::json response = biome.SaveBaseNoiseSettings()->ToJson();
            response.erase("BaseMask");
            response.erase("MaskTool");
            response["Mask"] = BuildMaskReference(biome.GetBiomeID().GetValue(), "base-noise");
            return response;
        }

        nlohmann::json BuildCustomBaseShapeResponse(BiomeManager &biome)
        {
            nlohmann::json response = biome.SaveCustomBaseShapeSettings()->ToJson();
            auto masks              = response.find("Masks");
            if (masks != response.end() && masks->is_array()) {
                for (std::size_t index = 0; index < masks->size(); ++index) {
                    nlohmann::json &mask = (*masks)[index];
                    if (!mask.is_object()) {
                        continue;
                    }
                    mask.erase("BaseMask");
                    mask.erase("MaskTool");
                    mask["Mask"] = BuildMaskReference(biome.GetBiomeID().GetValue(),
                                                      "custom-base-shape",
                                                      "MaskIndex",
                                                      std::to_string(index));
                }
            }
            return response;
        }

        nlohmann::json BuildBiomeGraph(BiomeManager &biome, int32_t order)
        {
            nlohmann::json filters = nlohmann::json::array();
            const auto &stack      = biome.GetFilters();
            for (std::size_t index = 0; index < stack.size(); ++index) {
                if (stack[index] != nullptr) {
                    filters.push_back(BuildFilterState(*stack[index],
                                                       static_cast<int32_t>(index),
                                                       biome.GetBiomeID().GetValue()));
                }
            }
            return {
                {"id", biome.GetBiomeID().GetValue()},
                {"order", order},
                {"general", biome.SaveGeneralSettings()->ToJson()},
                {"baseShape", BuildBaseShapeResponse(biome)},
                {"baseNoise", BuildBaseNoiseResponse(biome)},
                {"customBaseShape", BuildCustomBaseShapeResponse(biome)},
                {"mask", BuildMaskReference(biome.GetBiomeID().GetValue(), "biome")},
                {"filters", std::move(filters)}};
        }

        nlohmann::json BuildFilterSchemaLink(std::string_view definitionId)
        {
            return {
                {"operationId", "tf3d.generation.filter_catalog.get_schema"},
                {"parameters", {{"FilterDefinitionId", definitionId}}}};
        }

        ApiSchemaRuntimeProvider BuildGenerationGraphSchemaRuntime(
            tf3d::data::ApplicationState *applicationState)
        {
            return [applicationState](std::string_view name) -> std::optional<nlohmann::json> {
                GenerationManager *generation = GetGenerationManager(applicationState);
                if (generation == nullptr || generation->GetBiomes().empty() || generation->GetBiomes().front() == nullptr) {
                    return std::nullopt;
                }
                BiomeManager &biome = *generation->GetBiomes().front();

                if (name == "Generation.BaseNoiseInspector") {
                    return biome.GetBaseNoiseParameterSchema();
                }
                if (name == "Generation.BaseMask") {
                    return biome.GetBaseMaskSchema();
                }
                if (name == "Generation.FilterParameters") {
                    return nlohmann::json{
                        {"type", "object"},
                        {"description", "Filter-specific parameters are validated against the schema returned by get_schema for the filter's DefinitionID."},
                        {"additionalProperties", true}};
                }
                if (name == "Generation.AlgorithmSettings") {
                    return nlohmann::json{
                        {"type", "object"},
                        {"description", "Algorithm-specific settings are validated against the schema returned by get_schema for the selected AlgorithmID."},
                        {"additionalProperties", true}};
                }
                return std::nullopt;
            };
        }

        ApiResult GetBiomeDirectory(tf3d::data::ApplicationState *applicationState)
        {
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }
            return ApiResult::Success(BuildBiomeDirectory(*generation));
        }

        ApiResult CreateBiome(tf3d::data::ApplicationState *applicationState,
                              const nlohmann::json &arguments)
        {
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }
            std::string id;
            std::string error;
            if (!generation->CreateBiome(arguments.at("Name").get<std::string>(), id, error)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            return ApiResult::Success({{"id", id}, {"directory", BuildBiomeDirectory(*generation)}});
        }

        ApiResult GetBiome(tf3d::data::ApplicationState *applicationState,
                           const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const auto &entries = applicationState->generationManager->GetBiomes();
            const auto found    = std::find_if(entries.begin(), entries.end(), [biome](const auto &entry) {
                return entry.get() == biome;
            });
            return ApiResult::Success(BuildBiomeGraph(*biome, static_cast<int32_t>(std::distance(entries.begin(), found))));
        }

        ApiResult UpdateBiome(tf3d::data::ApplicationState *applicationState,
                              const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const auto settings = CreateSerializerNodeFromJson(arguments.at("State"));
            std::string error;
            if (!biome->LoadGeneralSettings(settings, error)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            return ApiResult::Success(biome->SaveGeneralSettings()->ToJson());
        }

        ApiResult MoveBiome(tf3d::data::ApplicationState *applicationState,
                            const nlohmann::json &arguments)
        {
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }
            std::string error;
            if (!generation->MoveBiome(arguments.at("BiomeId").get<std::string>(),
                                       arguments.at("TargetIndex").get<int32_t>(),
                                       error)) {
                const bool exists = generation->FindBiome(arguments.at("BiomeId").get<std::string>()) != nullptr;
                return ApiResult::Failure(exists ? ApiErrorType::InvalidArguments : ApiErrorType::BiomeNotFound,
                                          std::move(error));
            }
            return ApiResult::Success(BuildBiomeDirectory(*generation));
        }

        ApiResult RemoveBiome(tf3d::data::ApplicationState *applicationState,
                              const nlohmann::json &arguments)
        {
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }
            const std::string id = arguments.at("BiomeId").get<std::string>();
            std::string error;
            if (!generation->RemoveBiome(id, error)) {
                const bool exists = generation->FindBiome(id) != nullptr;
                return ApiResult::Failure(exists ? ApiErrorType::InvalidArguments : ApiErrorType::BiomeNotFound,
                                          std::move(error));
            }
            return ApiResult::Success(BuildBiomeDirectory(*generation));
        }

        ApiResult GetBaseShape(tf3d::data::ApplicationState *applicationState,
                               const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            return ApiResult::Success(BuildBaseShapeResponse(*biome));
        }

        ApiResult UpdateBaseShape(tf3d::data::ApplicationState *applicationState,
                                  const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const nlohmann::json &state = arguments.at("State");
            if (state.contains("AlgorithmSettings")) {
                const std::string algorithmId = state.value(
                    "AlgorithmID",
                    biome->SaveBaseShapeSettings()->ToJson().value("AlgorithmID", ""));
                const auto &algorithms = biome->GetBaseShapeGenerators();
                const auto algorithm   = std::find_if(algorithms.begin(), algorithms.end(), [&algorithmId](const auto &item) {
                    return item != nullptr && item->GetID() == algorithmId;
                });
                if (algorithm == algorithms.end()) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                              "AlgorithmSettings requires an available AlgorithmID.");
                }
                nlohmann::json validationErrors = nlohmann::json::array();
                if (!ApiSchemaTemplate::Validate(
                        state.at("AlgorithmSettings"), (*algorithm)->GetSettingsSchema(), validationErrors)) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                              "AlgorithmSettings do not match the selected algorithm schema.",
                                              {{"errors", std::move(validationErrors)}});
                }
            }
            std::string error;
            if (!biome->LoadBaseShapeSettings(CreateSerializerNodeFromJson(state), error)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            return ApiResult::Success(BuildBaseShapeResponse(*biome));
        }

        ApiResult GetBaseNoise(tf3d::data::ApplicationState *applicationState,
                               const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            return ApiResult::Success(BuildBaseNoiseResponse(*biome));
        }

        ApiResult ListBaseShapeAlgorithms(tf3d::data::ApplicationState *applicationState)
        {
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }

            nlohmann::json items = nlohmann::json::array();
            for (const auto &biome : generation->GetBiomes()) {
                if (biome == nullptr) {
                    continue;
                }
                for (const auto &algorithm : biome->GetBaseShapeGenerators()) {
                    if (algorithm == nullptr) {
                        continue;
                    }
                    items.push_back({{"id", algorithm->GetID()},
                                     {"name", algorithm->GetName()},
                                     {"description", algorithm->GetDescription()},
                                     {"schema", BuildBaseShapeAlgorithmSchemaLink(algorithm->GetID())}});
                }
                break;
            }
            const std::size_t count = items.size();
            return ApiResult::Success({{"items", std::move(items)}, {"count", count}});
        }

        ApiResult UpdateBaseNoise(tf3d::data::ApplicationState *applicationState,
                                  const nlohmann::json &arguments,
                                  const nlohmann::json &readOnlySchema)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const nlohmann::json &state = arguments.at("State");
            std::string validationError;
            if (!ApiSchemaTemplate::ValidateWritable(state, readOnlySchema, validationError)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(validationError));
            }
            std::string error;
            if (!biome->LoadBaseNoiseSettings(CreateSerializerNodeFromJson(state), error)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            return ApiResult::Success(BuildBaseNoiseResponse(*biome));
        }

        ApiResult GetCustomBaseShape(tf3d::data::ApplicationState *applicationState,
                                     const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            return ApiResult::Success(BuildCustomBaseShapeResponse(*biome));
        }

        ApiResult UpdateCustomBaseShape(tf3d::data::ApplicationState *applicationState,
                                        const nlohmann::json &arguments,
                                        const nlohmann::json &readOnlySchema)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const nlohmann::json &state = arguments.at("State");
            std::string validationError;
            if (!ApiSchemaTemplate::ValidateWritable(state, readOnlySchema, validationError)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(validationError));
            }
            std::string error;
            if (!biome->LoadCustomBaseShapeSettings(CreateSerializerNodeFromJson(state), error)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            return ApiResult::Success(BuildCustomBaseShapeResponse(*biome));
        }

        ApiResult GetFilterCatalog(tf3d::data::ApplicationState *applicationState,
                                   const nlohmann::json &arguments)
        {
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }
            const auto search    = Lower(arguments.value("Search", std::string{}));
            const int32_t limit  = std::clamp(arguments.value("Limit", 32), 1, 100);
            nlohmann::json items = nlohmann::json::array();
            for (const auto &biome : generation->GetBiomes()) {
                if (biome == nullptr) {
                    continue;
                }
                for (const auto &definition : biome->GetFilterDefinitions()) {
                    if (definition == nullptr) {
                        continue;
                    }
                    if (!search.empty() && Lower(definition->GetSearchText()).find(search) == std::string::npos) {
                        continue;
                    }
                    items.push_back({{"id", definition->GetID()},
                                     {"name", definition->GetName()},
                                     {"category", definition->GetCategory()},
                                     {"description", definition->GetDescription()},
                                     {"schema", BuildFilterSchemaLink(definition->GetID())}});
                    if (static_cast<int32_t>(items.size()) >= limit) {
                        break;
                    }
                }
                if (static_cast<int32_t>(items.size()) >= limit) {
                    break;
                }
                break;
            }
            return ApiResult::Success({{"items", std::move(items)}, {"limit", limit}, {"search", search}});
        }

        ApiResult GetFilterSchema(tf3d::data::ApplicationState *applicationState,
                                  const nlohmann::json &arguments)
        {
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }
            const std::string id  = arguments.at("FilterDefinitionId").get<std::string>();
            const auto definition = FindFilterDefinition(*generation, id);
            if (definition == nullptr) {
                return ApiResult::Failure(ApiErrorType::FilterDefinitionNotFound,
                                          "FilterDefinitionId does not exist in the filter catalog.");
            }
            return ApiResult::Success({{"id", definition->GetID()},
                                       {"name", definition->GetName()},
                                       {"category", definition->GetCategory()},
                                       {"description", definition->GetDescription()},
                                       {"parameters", definition->BuildParameterSchema()}});
        }

        ApiResult GetFilterStack(tf3d::data::ApplicationState *applicationState,
                                 const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            nlohmann::json filters = nlohmann::json::array();
            const auto &stack      = biome->GetFilters();
            for (std::size_t index = 0; index < stack.size(); ++index) {
                if (stack[index] != nullptr) {
                    filters.push_back(BuildFilterState(*stack[index],
                                                       static_cast<int32_t>(index),
                                                       biome->GetBiomeID().GetValue()));
                }
            }
            return ApiResult::Success({{"items", std::move(filters)}, {"count", stack.size()}});
        }

        ApiResult AddFilter(tf3d::data::ApplicationState *applicationState,
                            const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const int32_t oldCount = biome->GetFiltersCount();
            const int32_t target   = arguments.value("Position", oldCount);
            if (target < 0 || target > oldCount) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "Position must be between 0 and the current filter count.");
            }
            std::string filterId;
            std::string error;
            const int index = biome->AddFilterByDefinitionID(
                arguments.at("FilterDefinitionId").get<std::string>(), filterId, error);
            if (index < 0) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            if (target != index && !biome->ReorderFilter(filterId, target, error)) {
                biome->RemoveFilter(index);
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            const auto filter = biome->FindFilter(filterId);
            return filter != nullptr
                       ? ApiResult::Success(BuildFilterState(*filter, target, biome->GetBiomeID().GetValue()))
                       : ApiResult::Failure(ApiErrorType::TaskException, "The new filter could not be read back.");
        }

        ApiResult GetFilter(tf3d::data::ApplicationState *applicationState,
                            const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const std::string filterId = arguments.at("FilterId").get<std::string>();
            const auto filter          = biome->FindFilter(filterId);
            if (filter == nullptr) {
                return ApiResult::Failure(ApiErrorType::FilterNotFound,
                                          "FilterId does not exist in this biome.");
            }
            return ApiResult::Success(BuildFilterState(*filter,
                                                       FindFilterPosition(*biome, filterId),
                                                       biome->GetBiomeID().GetValue()));
        }

        ApiResult UpdateFilter(tf3d::data::ApplicationState *applicationState,
                               const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const std::string filterId = arguments.at("FilterId").get<std::string>();
            const auto filter          = biome->FindFilter(filterId);
            if (filter == nullptr) {
                return ApiResult::Failure(ApiErrorType::FilterNotFound,
                                          "FilterId does not exist in this biome.");
            }

            nlohmann::json state = arguments.at("State");
            if (state.contains("Parameters")) {
                nlohmann::json errors = nlohmann::json::array();
                if (!ApiSchemaTemplate::Validate(
                        state.at("Parameters"), filter->GetParameterSchema(), errors)) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                              "Parameters do not match this filter's composed schema.",
                                              {{"errors", std::move(errors)},
                                               {"schema", BuildFilterSchemaLink(filter->GetDefinitionID())}});
                }
            }
            if (state.contains("MergeMode")) {
                int32_t mergeMode = -1;
                if (!state.at("MergeMode").is_string() ||
                    !ParseMergeMode(state.at("MergeMode").get<std::string>(), mergeMode)) {
                    return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                              "MergeMode must be override, add, subtract, multiply, or blend.");
                }
                state["MergeMode"] = mergeMode;
            }

            std::string error;
            if (!biome->LoadFilter(filterId, CreateSerializerNodeFromJson(state), error)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            return ApiResult::Success(BuildFilterState(*filter,
                                                       FindFilterPosition(*biome, filterId),
                                                       biome->GetBiomeID().GetValue()));
        }

        ApiResult MoveFilter(tf3d::data::ApplicationState *applicationState,
                             const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const std::string filterId = arguments.at("FilterId").get<std::string>();
            std::string error;
            if (!biome->ReorderFilter(filterId,
                                      arguments.at("TargetIndex").get<int32_t>(),
                                      error)) {
                return ApiResult::Failure(biome->FindFilter(filterId) != nullptr
                                              ? ApiErrorType::InvalidArguments
                                              : ApiErrorType::FilterNotFound,
                                          std::move(error));
            }
            return GetFilter(applicationState, arguments);
        }

        ApiResult RemoveFilter(tf3d::data::ApplicationState *applicationState,
                               const nlohmann::json &arguments)
        {
            BiomeManager *biome = nullptr;
            ApiResult failure;
            if (!ResolveBiome(applicationState, arguments, biome, failure)) {
                return failure;
            }
            const std::string filterId = arguments.at("FilterId").get<std::string>();
            const int32_t position     = FindFilterPosition(*biome, filterId);
            if (position < 0) {
                return ApiResult::Failure(ApiErrorType::FilterNotFound,
                                          "FilterId does not exist in this biome.");
            }
            if (!biome->RemoveFilter(position)) {
                return ApiResult::Failure(ApiErrorType::TaskException,
                                          "The filter could not be removed.");
            }
            return GetFilterStack(applicationState, arguments);
        }

        ApiResult GetBiomeMixer(tf3d::data::ApplicationState *applicationState)
        {
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }
            nlohmann::json response = generation->SaveBiomeMixerSettings()->ToJson();
            response["Methods"]     = nlohmann::json::array({{{"id", "simple"}, {"name", "Simple"}, {"available", true}},
                                                             {{"id", "alphaBlend"}, {"name", "Alpha Blend"}, {"available", false}, {"reason", "The Alpha Blend mixer is not implemented yet."}}});
            return ApiResult::Success(std::move(response));
        }

        ApiResult UpdateBiomeMixer(tf3d::data::ApplicationState *applicationState,
                                   const nlohmann::json &arguments)
        {
            GenerationManager *generation = GetGenerationManager(applicationState);
            if (generation == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
            }
            const nlohmann::json &state = arguments.at("State");
            std::string error;
            if (!generation->LoadBiomeMixerSettings(CreateSerializerNodeFromJson(state), error)) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            return GetBiomeMixer(applicationState);
        }

    } // namespace

    void RegisterGenerationGraphOperations(OperationRegistry &operations,
                                           tf3d::data::ApplicationState *applicationState)
    {
        const ApiSchemaRuntimeProvider runtime = BuildGenerationGraphSchemaRuntime(applicationState);
        const auto baseNoiseReadOnly           = ApiSchemaTemplate::ComposeDefault(
            "Operations/Generation/BaseNoiseStateReadOnly.json", runtime);
        const auto customShapeReadOnly = ApiSchemaTemplate::ComposeDefault(
            "Operations/Generation/CustomBaseShapeStateReadOnly.json");

        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/ListBiomes.json",
            [applicationState](const nlohmann::json &) { return GetBiomeDirectory(applicationState); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/CreateBiome.json",
            [applicationState](const nlohmann::json &arguments) { return CreateBiome(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/GetBiome.json",
            [applicationState](const nlohmann::json &arguments) { return GetBiome(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/UpdateBiome.json",
            [applicationState](const nlohmann::json &arguments) { return UpdateBiome(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/MoveBiome.json",
            [applicationState](const nlohmann::json &arguments) { return MoveBiome(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/RemoveBiome.json",
            [applicationState](const nlohmann::json &arguments) { return RemoveBiome(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/GetBaseShape.json",
            [applicationState](const nlohmann::json &arguments) { return GetBaseShape(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(
            operations,
            "Operations/Generation/Definitions/UpdateBaseShape.json",
            runtime,
            [applicationState](const nlohmann::json &arguments) { return UpdateBaseShape(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(
            operations,
            "Operations/Generation/Definitions/GetBaseNoise.json",
            runtime,
            [applicationState](const nlohmann::json &arguments) { return GetBaseNoise(applicationState, arguments); });
        if (baseNoiseReadOnly) {
            TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(
                operations,
                "Operations/Generation/Definitions/UpdateBaseNoise.json",
                runtime,
                [applicationState, readOnly = *baseNoiseReadOnly](const nlohmann::json &arguments) {
                    return UpdateBaseNoise(applicationState, arguments, readOnly);
                });
        }
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/GetCustomBaseShape.json",
            [applicationState](const nlohmann::json &arguments) { return GetCustomBaseShape(applicationState, arguments); });
        if (customShapeReadOnly) {
            TF3D_API_REGISTER_OPERATION(
                operations,
                "Operations/Generation/Definitions/UpdateCustomBaseShape.json",
                [applicationState, readOnly = *customShapeReadOnly](const nlohmann::json &arguments) {
                    return UpdateCustomBaseShape(applicationState, arguments, readOnly);
                });
        }
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/ListFilters.json",
            [applicationState](const nlohmann::json &arguments) { return GetFilterCatalog(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/GetFilterSchema.json",
            [applicationState](const nlohmann::json &arguments) { return GetFilterSchema(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/ListBaseShapeAlgorithms.json",
            [applicationState](const nlohmann::json &) { return ListBaseShapeAlgorithms(applicationState); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/GetBaseShapeAlgorithmSchema.json",
            [applicationState](const nlohmann::json &arguments) {
                GenerationManager *generation = GetGenerationManager(applicationState);
                if (generation == nullptr) {
                    return ApiResult::Failure(ApiErrorType::Shutdown, "Generation is not available.");
                }
                const std::string algorithmId = arguments.at("AlgorithmId").get<std::string>();
                for (const auto &biome : generation->GetBiomes()) {
                    if (biome == nullptr) {
                        continue;
                    }
                    const auto &algorithms = biome->GetBaseShapeGenerators();
                    const auto algorithm   = std::find_if(algorithms.begin(), algorithms.end(), [&algorithmId](const auto &item) {
                        return item != nullptr && item->GetID() == algorithmId;
                    });
                    if (algorithm != algorithms.end()) {
                        return ApiResult::Success({{"id", (*algorithm)->GetID()},
                                                   {"name", (*algorithm)->GetName()},
                                                   {"description", (*algorithm)->GetDescription()},
                                                   {"settings", (*algorithm)->GetSettingsSchema()}});
                    }
                }
                return ApiResult::Failure(ApiErrorType::InvalidArguments,
                                          "AlgorithmId does not exist in the base-shape catalog.");
            });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/GetFilterStack.json",
            [applicationState](const nlohmann::json &arguments) { return GetFilterStack(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/AddFilter.json",
            [applicationState](const nlohmann::json &arguments) { return AddFilter(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/GetFilter.json",
            [applicationState](const nlohmann::json &arguments) { return GetFilter(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION_WITH_RUNTIME(
            operations,
            "Operations/Generation/Definitions/UpdateFilter.json",
            runtime,
            [applicationState](const nlohmann::json &arguments) { return UpdateFilter(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/MoveFilter.json",
            [applicationState](const nlohmann::json &arguments) { return MoveFilter(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/RemoveFilter.json",
            [applicationState](const nlohmann::json &arguments) { return RemoveFilter(applicationState, arguments); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/GetBiomeMixer.json",
            [applicationState](const nlohmann::json &) { return GetBiomeMixer(applicationState); });
        TF3D_API_REGISTER_OPERATION(
            operations,
            "Operations/Generation/Definitions/UpdateBiomeMixer.json",
            [applicationState](const nlohmann::json &arguments) { return UpdateBiomeMixer(applicationState, arguments); });
    }

} // namespace tf3d::api
