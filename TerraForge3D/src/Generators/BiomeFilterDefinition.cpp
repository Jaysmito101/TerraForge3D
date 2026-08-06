#include "Generators/BiomeFilterDefinition.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Utils/Utils.h"

#include <algorithm>

namespace tf3d::generators
{

    std::shared_ptr<BiomeFilterDefinition> BiomeFilterDefinition::LoadFromFolder(
        const std::filesystem::path &folder,
        const std::filesystem::path &shaderRoot)
    {
        const auto metadataPath          = folder / "filter.json";
        bool loaded                      = false;
        const std::string metadataSource = ReadShaderSourceFile(metadataPath.string(), &loaded);
        if (!loaded) {
            TF3D_LOG_ERROR("Failed to read filter metadata '{}'.", metadataPath.string());
            return nullptr;
        }

        nlohmann::json metadata;
        try {
            metadata = nlohmann::json::parse(metadataSource);
        } catch (const nlohmann::json::parse_error &exception) {
            TF3D_LOG_ERROR("Failed to parse filter metadata '{}': {}", metadataPath.string(), exception.what());
            return nullptr;
        }

        std::error_code relativeError;
        const auto relativeFolder = std::filesystem::relative(folder, shaderRoot, relativeError);
        if (relativeError || relativeFolder.empty()) {
            TF3D_LOG_ERROR("Failed to resolve filter folder '{}' relative to '{}'.", folder.string(), shaderRoot.string());
            return nullptr;
        }

        auto definition                = std::shared_ptr<BiomeFilterDefinition>(new BiomeFilterDefinition());
        definition->m_Metadata         = metadata;
        definition->m_FolderShaderPath = relativeFolder.generic_string();
        definition->m_ID               = metadata.value("ID", folder.filename().string());
        definition->m_Name             = metadata.value("Name", definition->m_ID);
        definition->m_Category         = metadata.value("Category", "Other");
        definition->m_Description      = metadata.value("Description", "");
        definition->m_Implementation   = metadata.value("Implementation", "Unknown");
        definition->m_SearchText       = definition->m_Category + " " + definition->m_Name + " " + definition->m_ID;

        if (metadata.contains("Phases") && metadata["Phases"].is_object()) {
            for (const auto &[phase, phaseValue] : metadata["Phases"].items()) {
                if (!phaseValue.is_string())
                    continue;
                std::filesystem::path phasePath = phaseValue.get<std::string>();
                if (phasePath.extension() == ".glsl")
                    phasePath.replace_extension();
                definition->m_PhaseShaderPaths[phase] =
                    (std::filesystem::path(definition->m_FolderShaderPath) / phasePath).generic_string();
            }
        }

        if (definition->m_ID.empty() || definition->m_Name.empty() || definition->m_Implementation == "Unknown") {
            TF3D_LOG_ERROR("Filter metadata '{}' is missing ID, Name, or Implementation.", metadataPath.string());
            return nullptr;
        }
        return definition;
    }

    bool BiomeFilterDefinition::BuildInspector(CustomInspector &inspector) const
    {
        return inspector.LoadConfig(m_Metadata);
    }

    bool BiomeFilterDefinition::NeedsFieldStatistics() const
    {
        const auto statistics = m_Metadata.value("Statistics", nlohmann::json::object());
        return statistics.is_object() &&
               (statistics.value("NeedsMinMax", false) || statistics.value("NeedsHistogram", false));
    }

    bool BiomeFilterDefinition::NeedsHistogram() const
    {
        const auto statistics = m_Metadata.value("Statistics", nlohmann::json::object());
        return statistics.is_object() && statistics.value("NeedsHistogram", false);
    }

    std::string BiomeFilterDefinition::GetRequestedPercentileParameter() const
    {
        const auto statistics = m_Metadata.value("Statistics", nlohmann::json::object());
        return statistics.is_object() ? statistics.value("RequestedPercentileParameter", "") : "";
    }

    std::shared_ptr<ComputeShader> BiomeFilterDefinition::GetPhaseShader(ApplicationState *appState, const std::string &phase) const
    {
        const auto path = m_PhaseShaderPaths.find(phase);
        if (path == m_PhaseShaderPaths.end())
            return nullptr;

        const auto cached = m_PhaseShaders.find(phase);
        if (cached != m_PhaseShaders.end())
            return cached->second;

        bool loaded = false;
        auto shader = appState->resourceManager->LoadComputeShader(path->second, false, &loaded);
        if (!loaded || shader == nullptr) {
            TF3D_LOG_ERROR("Failed to load filter phase '{}' for '{}'.", phase, m_ID);
            return nullptr;
        }
        m_PhaseShaders[phase] = shader;
        return shader;
    }

} // namespace tf3d::generators
