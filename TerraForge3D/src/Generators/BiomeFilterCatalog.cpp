#include "Generators/BiomeFilterCatalog.h"

#include "Data/ApplicationState.h"

#include <algorithm>
#include <filesystem>
#include <unordered_set>

BiomeFilterCatalog::BiomeFilterCatalog(ApplicationState *appState)
    : m_AppState(appState)
{
    Reload();
}

bool BiomeFilterCatalog::Reload()
{
    m_Definitions.clear();
    const std::filesystem::path root = std::filesystem::path(m_AppState->constants.shadersDir) / "generation" / "filters";
    std::error_code error;
    if (!std::filesystem::exists(root, error)) {
        TF3D_LOG_WARN("Filter catalog directory '{}' does not exist.", root.string());
        return false;
    }

    std::unordered_set<std::string> ids;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(root, error)) {
        if (error)
            break;
        if (!entry.is_regular_file() || entry.path().filename() != "filter.json")
            continue;

        auto definition = BiomeFilterDefinition::LoadFromFolder(entry.path().parent_path(), root.parent_path().parent_path());
        if (definition == nullptr)
            continue;
        if (!ids.insert(definition->GetID()).second) {
            TF3D_LOG_ERROR("Duplicate filter definition ID '{}'.", definition->GetID());
            continue;
        }
        m_Definitions.push_back(definition);
    }

    std::sort(m_Definitions.begin(), m_Definitions.end(), [](const auto &left, const auto &right) {
        if (left->GetCategory() != right->GetCategory())
            return left->GetCategory() < right->GetCategory();
        return left->GetName() < right->GetName();
    });
    TF3D_LOG_INFO("Loaded {} filter definitions.", m_Definitions.size());
    return !m_Definitions.empty();
}

std::shared_ptr<BiomeFilterDefinition> BiomeFilterCatalog::FindByID(const std::string &id) const
{
    for (const auto &definition : m_Definitions)
        if (definition->GetID() == id)
            return definition;
    return nullptr;
}
