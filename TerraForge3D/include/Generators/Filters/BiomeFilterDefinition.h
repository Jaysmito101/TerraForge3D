#pragma once

#include "Base/Base.h"
#include "Inspector/CustomInspector.h"
#include "Generators/Filters/BiomeFilterTypes.h"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <unordered_map>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)
TF3D_FWD_DEC_CLASS(ComputeShader, tf3d::base)

namespace tf3d::generators
{

    struct BiomeFilterStatisticsRequirements {
        bool needsMinMax    = false;
        bool needsHistogram = false;
        std::string requestedPercentileParameter;
    };

    struct BiomeFilterRuntimeConfig {
        BiomeFilterImplementation implementation = BiomeFilterImplementation::Unknown;
        BiomeFilterMergeMode defaultMergeMode    = BiomeFilterMergeMode::Blend;
        BiomeFilterStatisticsRequirements statistics;
        nlohmann::json resources = nlohmann::json::object();
        nlohmann::json execution = nlohmann::json::object();
    };

    class BiomeFilterDefinition
    {
    public:
        static std::shared_ptr<BiomeFilterDefinition> LoadFromConfig(const nlohmann::json &config);

        bool BuildInspector(inspector::CustomInspector &inspector) const;
        base::ComputeShader *GetPhaseShader(data::ApplicationState *appState, const std::string &phase) const;

        inline const BiomeFilterRuntimeConfig &GetRuntime() const
        {
            return m_Runtime;
        }

        inline bool NeedsFieldStatistics() const
        {
            return m_Runtime.statistics.needsMinMax || m_Runtime.statistics.needsHistogram;
        }

        inline bool NeedsHistogram() const
        {
            return m_Runtime.statistics.needsHistogram;
        }

        inline const std::string &GetRequestedPercentileParameter() const
        {
            return m_Runtime.statistics.requestedPercentileParameter;
        }

        inline const std::string &GetID() const
        {
            return m_ID;
        }
        inline const std::string &GetName() const
        {
            return m_Name;
        }
        inline const std::string &GetCategory() const
        {
            return m_Category;
        }
        inline const std::string &GetDescription() const
        {
            return m_Description;
        }
        inline BiomeFilterImplementation GetImplementation() const
        {
            return m_Runtime.implementation;
        }
        inline const std::string &GetSearchText() const
        {
            return m_SearchText;
        }

    private:
        BiomeFilterDefinition() = default;

        std::string m_ID;
        std::string m_Name;
        std::string m_Category = "Other";
        std::string m_Description;
        std::string m_SearchText;
        BiomeFilterRuntimeConfig m_Runtime;
        std::unordered_map<std::string, std::string> m_PhaseShaderPaths;
        mutable std::unordered_map<std::string, base::ComputeShader> m_PhaseShaders;
        nlohmann::json m_InspectorConfig = nlohmann::json::object();
    };

} // namespace tf3d::generators
