#include "Generators/Filters/BiomeFilterDefinition.h"

#include "Data/ApplicationState.h"
#include "Data/ResourceManager.h"
#include "Utils/Utils.h"

#include <algorithm>
#include <filesystem>

namespace tf3d::generators
{

    std::shared_ptr<BiomeFilterDefinition> BiomeFilterDefinition::LoadFromConfig(const nlohmann::json &config)
    {
        if (!config.is_object()) {
            TF3D_LOG_ERROR("Failed to load filter metadata: expected an object.");
            return nullptr;
        }

        const auto runtime   = config.value("Runtime", nlohmann::json::object());
        const auto inspector = config.value("Inspector", nlohmann::json::object());
        if (!runtime.is_object() || !inspector.is_object()) {
            TF3D_LOG_ERROR("Filter metadata must contain Runtime and Inspector objects.");
            return nullptr;
        }

        auto definition               = std::shared_ptr<BiomeFilterDefinition>(new BiomeFilterDefinition());
        definition->m_ID              = config.value("ID", "");
        definition->m_Name            = config.value("Name", definition->m_ID);
        definition->m_Category        = config.value("Category", "Other");
        definition->m_Description     = config.value("Description", "");
        definition->m_InspectorConfig = inspector;
        definition->m_SearchText      = definition->m_Category + " " + definition->m_Name + " " +
                                   definition->m_ID + " " + definition->m_Description;

        definition->m_Runtime.implementation =
            BiomeFilterImplementationFromString(runtime.value("Implementation", ""));
        if (definition->m_Runtime.implementation == BiomeFilterImplementation::Unknown) {
            TF3D_LOG_ERROR("Filter '{}' has an unknown runtime implementation.", definition->m_ID);
            return nullptr;
        }

        const auto defaults = runtime.value("Defaults", nlohmann::json::object());
        if (!defaults.is_object()) {
            TF3D_LOG_ERROR("Filter '{}' has an invalid Runtime.Defaults object.", definition->m_ID);
            return nullptr;
        }
        const std::string defaultMergeMode     = defaults.value("MergeMode", "Blend");
        definition->m_Runtime.defaultMergeMode = BiomeFilterMergeModeFromString(defaultMergeMode);
        if (definition->m_Runtime.defaultMergeMode == BiomeFilterMergeMode::Blend &&
            defaultMergeMode != "Blend") {
            TF3D_LOG_ERROR("Filter '{}' has an unknown default merge mode '{}'.",
                           definition->m_ID,
                           defaultMergeMode);
            return nullptr;
        }

        const auto statistics = runtime.value("Statistics", nlohmann::json::object());
        if (!statistics.is_object()) {
            TF3D_LOG_ERROR("Filter '{}' has an invalid Runtime.Statistics object.", definition->m_ID);
            return nullptr;
        }
        definition->m_Runtime.statistics.needsMinMax    = statistics.value("NeedsMinMax", false);
        definition->m_Runtime.statistics.needsHistogram = statistics.value("NeedsHistogram", false);
        definition->m_Runtime.statistics.requestedPercentileParameter =
            statistics.value("RequestedPercentileParameter", "");

        const auto shaders              = runtime.value("Shaders", nlohmann::json::object());
        definition->m_Runtime.resources = runtime.value("Resources", nlohmann::json::object());
        definition->m_Runtime.execution = runtime.value("Execution", nlohmann::json::object());
        if (!shaders.is_object() ||
            !definition->m_Runtime.resources.is_object() ||
            !definition->m_Runtime.execution.is_object()) {
            TF3D_LOG_ERROR("Filter '{}' has an invalid Runtime shader, resource, or execution object.",
                           definition->m_ID);
            return nullptr;
        }

        const auto phases = shaders.value("Phases", nlohmann::json::object());
        if (!phases.is_object()) {
            TF3D_LOG_ERROR("Filter '{}' has an invalid Runtime.Shaders.Phases object.", definition->m_ID);
            return nullptr;
        }
        for (const auto &[phase, phaseValue] : phases.items()) {
            if (!phaseValue.is_string()) {
                continue;
            }
            std::filesystem::path phasePath = phaseValue.get<std::string>();
            if (phasePath.empty() || phasePath.is_absolute()) {
                TF3D_LOG_ERROR("Filter '{}' has an invalid shader path for phase '{}'.",
                               definition->m_ID,
                               phase);
                continue;
            }
            if (phasePath.extension() == ".glsl") {
                phasePath.replace_extension();
            }
            definition->m_PhaseShaderPaths[phase] = phasePath.generic_string();
        }

        if (definition->m_ID.empty() || definition->m_Name.empty()) {
            TF3D_LOG_ERROR("Filter metadata is missing ID or Name.");
            return nullptr;
        }
        return definition;
    }

    bool BiomeFilterDefinition::BuildInspector(inspector::CustomInspector &inspector) const
    {
        return inspector.LoadConfig(m_InspectorConfig);
    }

    ComputeShader *BiomeFilterDefinition::GetPhaseShader(ApplicationState *appState, const std::string &phase) const
    {
        const auto path = m_PhaseShaderPaths.find(phase);
        if (path == m_PhaseShaderPaths.end())
            return nullptr;

        const auto cached = m_PhaseShaders.find(phase);
        if (cached != m_PhaseShaders.end())
            return &cached->second;

        bool loaded = false;
        auto shader = appState->resourceManager->LoadComputeShader(path->second, false, &loaded);
        if (!loaded || !shader) {
            TF3D_LOG_ERROR("Failed to load filter phase '{}' for '{}'.", phase, m_ID);
            return nullptr;
        }

        auto [iterator, inserted] = m_PhaseShaders.emplace(phase, std::move(*shader));
        return &iterator->second;
    }

} // namespace tf3d::generators
