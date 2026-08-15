#include "Generators/Filters/BiomeFilterCatalog.h"

#include "Data/ApplicationState.h"
#include "Inspector/CustomInspector.h"
#include "Utils/JsonIncludeResolver.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <filesystem>
#include <unordered_set>

namespace tf3d::generators
{

    BiomeFilterCatalog::BiomeFilterCatalog(ApplicationState *appState)
        : m_AppState(appState)
    {
        Reload();
    }

    bool BiomeFilterCatalog::Reload()
    {
        m_Definitions.clear();
        if (m_AppState == nullptr) {
            TF3D_LOG_ERROR("Cannot load filter catalog without application state.");
            return false;
        }

        const auto dataDirectory = std::filesystem::path(m_AppState->constants.dataDir);
        const auto inspectorPath = inspector::CustomInspector::GetConfigPath(dataDirectory, "Filters");
        utils::JsonIncludeResolverOptions resolverOptions;
        resolverOptions.rootDirectory  = dataDirectory / "inspectors";
        resolverOptions.pathMode       = utils::JsonIncludePathMode::RelativeToIncludingFile;
        resolverOptions.restrictToRoot = true;
        const utils::JsonIncludeResolver resolver(resolverOptions);

        std::string resolveError;
        const auto catalog = resolver.ResolveFile(inspectorPath, &resolveError);
        if (!catalog) {
            TF3D_LOG_ERROR("Failed to load filter inspector '{}': {}", inspectorPath.string(), resolveError);
            return false;
        }
        if (!catalog->is_object() || !catalog->contains("Sections") || !(*catalog)["Sections"].is_array()) {
            TF3D_LOG_ERROR("Filter inspector '{}' must contain a Sections array.", inspectorPath.string());
            return false;
        }

        std::unordered_set<std::string> ids;
        for (size_t index = 0; index < (*catalog)["Sections"].size(); ++index) {
            const auto &config = (*catalog)["Sections"][index];
            if (!config.is_object()) {
                TF3D_LOG_WARN("Skipping filter inspector section {}: expected an object.", index);
                continue;
            }

            auto definition = BiomeFilterDefinition::LoadFromConfig(config);
            if (definition == nullptr)
                continue;
            if (!ids.insert(utils::CanonicalID(definition->GetID())).second) {
                TF3D_LOG_ERROR("Duplicate filter definition ID '{}'.", definition->GetID());
                continue;
            }
            m_Definitions.push_back(definition);
        }

        std::sort(m_Definitions.begin(), m_Definitions.end(), [](const auto &left, const auto &right) {
            if (left->GetCategory() != right->GetCategory()) {
                return left->GetCategory() < right->GetCategory();
            }
            return left->GetName() < right->GetName();
        });
        TF3D_LOG_INFO("Loaded {} filter definitions.", m_Definitions.size());
        return !m_Definitions.empty();
    }

    std::shared_ptr<BiomeFilterDefinition> BiomeFilterCatalog::FindByID(const std::string &id) const
    {
        for (const auto &definition : m_Definitions) {
            if (definition->GetID() == id) {
                return definition;
            }
        }
        return nullptr;
    }

} // namespace tf3d::generators
