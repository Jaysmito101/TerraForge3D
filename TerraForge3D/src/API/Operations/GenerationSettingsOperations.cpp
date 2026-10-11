#include "API/Operations/GenerationSettingsOperations.h"

#include "API/ApiResult.h"
#include "API/OperationRegistry.h"
#include "API/SchemaTemplate.h"
#include "Data/ApplicationState.h"
#include "Exporters/Serializer.h"
#include "Generators/GenerationManager.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace tf3d::api
{

    namespace
    {
        using exporters::CreateSerializerNode;
        using exporters::SerializerNode;
        using generators::GenerationGlobalSettingsApplyResult;
        using generators::GenerationSeedTextureSource;
        using generators::GeneratorDataStorage;

        template <typename Enum>
        struct EnumFlagName {
            Enum flag;
            std::string_view name;
        };

        constexpr std::array<EnumFlagName<generators::GenerationDirtyScope>, 4> kDirtyScopeNames = {{
            {generators::GenerationDirtyScope::Biomes, "biomes"},
            {generators::GenerationDirtyScope::Mixer, "mixer"},
            {generators::GenerationDirtyScope::Force, "force"},
            {generators::GenerationDirtyScope::AllBiomes, "allBiomes"},
        }};

        constexpr std::array<EnumFlagName<generators::GenerationDirtyCause>, 5> kDirtyCauseNames = {{
            {generators::GenerationDirtyCause::UiEdit, "uiEdit"},
            {generators::GenerationDirtyCause::FileLoad, "fileLoad"},
            {generators::GenerationDirtyCause::Resize, "resize"},
            {generators::GenerationDirtyCause::External, "external"},
            {generators::GenerationDirtyCause::Force, "force"},
        }};

        template <typename Enum, std::size_t Count>
        nlohmann::json EnumFlagsToJson(uint32_t mask,
                                       const std::array<EnumFlagName<Enum>, Count> &names,
                                       std::string_view unknownName = {})
        {
            auto values = nlohmann::json::array();
            for (const auto &[flag, name] : names) {
                if ((mask & static_cast<uint32_t>(flag)) != 0) {
                    values.push_back(name);
                }
            }
            if (values.empty() && mask != 0 && !unknownName.empty()) {
                values.push_back(unknownName);
            }
            return values;
        }

        std::string StorageName(GeneratorDataStorage storage)
        {
            return storage == GeneratorDataStorage::R16F ? "R16F" : "R32F";
        }

        std::string SeedSourceName(GenerationSeedTextureSource source)
        {
            return source == GenerationSeedTextureSource::MainModel ? "mainModel" : "none";
        }

        nlohmann::json BuildSettingsResponse(const SerializerNode &settings,
                                             const SerializerNode &status)
        {
            using SerializerChild = exporters::SerializerNode;

            nlohmann::json response       = settings->ToJson();
            const auto tileStatus         = status->Get<SerializerChild>("tile");
            const auto limits             = status->Get<SerializerChild>("limits");
            const auto dirty              = status->Get<SerializerChild>("dirty");
            const auto fieldStorageStatus = status->Get<SerializerChild>("fieldStorage");
            const auto seedTexture        = settings->Get<SerializerChild>("seedTexture");
            const uint32_t scopeMask      = dirty->Get<uint32_t>("scopeMask");
            const uint32_t causeMask      = dirty->Get<uint32_t>("causeMask");

            response["tile"]["count"]         = tileStatus->Get<int32_t>("count");
            response["tile"]["mapResolution"] = tileStatus->Get<int32_t>("mapResolution");
            response["limits"]                = limits->ToJson();
            response["dirty"]                 = {
                {"isDirty", dirty->Get<bool>("isDirty")},
                {"revision", dirty->Get<std::string>("revision")},
                {"scopes", EnumFlagsToJson(scopeMask, kDirtyScopeNames)},
                {"causes", EnumFlagsToJson(causeMask, kDirtyCauseNames, "unknown")}};
            response["seedTexture"]["source"] =
                SeedSourceName(seedTexture->Get<GenerationSeedTextureSource>("source"));
            response["fieldStorage"] = {
                {"active", StorageName(fieldStorageStatus->Get<GeneratorDataStorage>("active"))},
                {"requested", StorageName(settings->Get<GeneratorDataStorage>("fieldStorage"))},
                {"restartRequired", fieldStorageStatus->Get<bool>("restartRequired")}};
            return response;
        }

        ApiResult GetGenerationSettings(tf3d::data::ApplicationState *applicationState)
        {
            if (applicationState == nullptr || applicationState->generationManager == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown,
                                          "Generation settings are not available.");
            }
            const auto settings = applicationState->generationManager->SaveGlobalSettings();
            const auto status   = applicationState->generationManager->GetGlobalSettingsStatus();
            return ApiResult::Success(BuildSettingsResponse(settings, status));
        }

        SerializerNode BuildSettingsUpdateNode(const nlohmann::json &arguments)
        {
            auto update = CreateSerializerNode();

            const auto tile = arguments.find("tile");
            if (tile != arguments.end()) {
                auto tileSettings = CreateSerializerNode();
                if (tile->contains("resolution")) {
                    tileSettings->Set("resolution", tile->at("resolution").get<int32_t>());
                }
                if (tile->contains("size")) {
                    tileSettings->Set("size", tile->at("size").get<float>());
                }
                update->Set("tile", tileSettings);
            }

            if (arguments.contains("autoUpdatePaused")) {
                update->Set("autoUpdatePaused", arguments.at("autoUpdatePaused").get<bool>());
            }

            const auto seedTexture = arguments.find("seedTexture");
            if (seedTexture != arguments.end()) {
                auto seedTextureSettings = CreateSerializerNode();
                if (seedTexture->contains("enabled")) {
                    seedTextureSettings->Set("enabled", seedTexture->at("enabled").get<bool>());
                }
                if (seedTexture->contains("resolution")) {
                    seedTextureSettings->Set("resolution", seedTexture->at("resolution").get<int32_t>());
                }
                if (seedTexture->contains("source")) {
                    const std::string source = seedTexture->at("source").get<std::string>();
                    seedTextureSettings->Set(
                        "source",
                        source == "mainModel" ? GenerationSeedTextureSource::MainModel
                                              : GenerationSeedTextureSource::None);
                }
                update->Set("seedTexture", seedTextureSettings);
            }

            if (arguments.contains("fieldStorage")) {
                const std::string storage = arguments.at("fieldStorage").get<std::string>();
                update->Set("fieldStorage",
                            storage == "R16F" ? GeneratorDataStorage::R16F
                                              : GeneratorDataStorage::R32F);
            }

            return update;
        }

        ApiResult UpdateGenerationSettings(tf3d::data::ApplicationState *applicationState,
                                           const nlohmann::json &arguments)
        {
            if (applicationState == nullptr || applicationState->generationManager == nullptr) {
                return ApiResult::Failure(ApiErrorType::Shutdown,
                                          "Generation settings are not available.");
            }

            const auto update = BuildSettingsUpdateNode(arguments);
            std::string error;

            const auto result = applicationState->generationManager->LoadGlobalSettings(update, error);
            if (result == GenerationGlobalSettingsApplyResult::Invalid) {
                return ApiResult::Failure(ApiErrorType::InvalidArguments, std::move(error));
            }
            if (result == GenerationGlobalSettingsApplyResult::PersistenceFailed) {
                return ApiResult::Failure(ApiErrorType::TaskException, std::move(error));
            }

            const auto settings = applicationState->generationManager->SaveGlobalSettings();
            const auto status   = applicationState->generationManager->GetGlobalSettingsStatus();
            return ApiResult::Success(BuildSettingsResponse(settings, status));
        }
    } // namespace

    void RegisterGenerationSettingsOperations(OperationRegistry &operations,
                                              tf3d::data::ApplicationState *applicationState)
    {
        RegisterOperationFromJson(
            operations,
            ApiSchemaTemplate::ComposeDefault("Operations/Generation/Definitions/GetSettings.json"),
            [applicationState](const nlohmann::json &) {
                return GetGenerationSettings(applicationState);
            });

        RegisterOperationFromJson(
            operations,
            ApiSchemaTemplate::ComposeDefault("Operations/Generation/Definitions/UpdateSettings.json"),
            [applicationState](const nlohmann::json &arguments) {
                return UpdateGenerationSettings(applicationState, arguments);
            });
    }

} // namespace tf3d::api
