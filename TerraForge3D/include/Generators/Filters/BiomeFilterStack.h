#pragma once

#include "Generators/Filters/BiomeFilter.h"
#include "Generators/Filters/BiomeFilterCatalog.h"
#include "Generators/GeneratorDataStatistics.h"

#include <vector>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class BiomeFilterStack
    {
    public:
        BiomeFilterStack(ApplicationState *appState);
        ~BiomeFilterStack() = default;

        void Resize(size_t dataSize, int resolution);
        void Update(GeneratorData *baseResult);
        bool ShowSettings(int filterIndex);
        int AddFilter(const std::shared_ptr<BiomeFilterDefinition> &definition);
        bool RemoveFilter(int filterIndex);
        void Load(SerializerNode data);
        SerializerNode Save() const;

        inline const std::vector<std::shared_ptr<BiomeFilter>> &GetFilters() const
        {
            return m_Filters;
        }
        inline const std::vector<std::shared_ptr<BiomeFilterDefinition>> &GetDefinitions() const
        {
            return m_Catalog->GetDefinitions();
        }
        inline bool IsUpdationRequired() const
        {
            return m_RequireUpdation;
        }

    private:
        void RunFilter(const std::shared_ptr<BiomeFilter> &filter, GeneratorData *input, GeneratorData *output);
        void RunPhase(const std::shared_ptr<BiomeFilter> &filter, const nlohmann::json &pass,
                      GeneratorData *input, GeneratorData *output, GeneratorData *reference);
        void RunMergePhase(const std::shared_ptr<BiomeFilter> &filter, const nlohmann::json &merge,
                           GeneratorData *input, GeneratorData *operation, GeneratorData *output);
        void SetPassUniforms(const std::shared_ptr<BiomeFilter> &filter,
                             base::ComputeShader *shader,
                             const nlohmann::json &bindings);
        void BindFieldStatistics(const std::shared_ptr<BiomeFilter> &filter, base::ComputeShader *shader);
        void EnsureTempBufferCount(size_t count);

        static constexpr int FieldStatisticsBinding = 4;

        data::ApplicationState *m_AppState = nullptr;
        std::shared_ptr<BiomeFilterCatalog> m_Catalog;
        std::vector<std::shared_ptr<GeneratorData>> m_TempBuffers;
        std::shared_ptr<GeneratorData> m_ResultA;
        std::shared_ptr<GeneratorData> m_ResultB;
        std::shared_ptr<GeneratorDataStatistics> m_Statistics;
        std::vector<std::shared_ptr<BiomeFilter>> m_Filters;

        size_t m_DataSize            = 0;
        int m_Resolution             = 1;
        int m_StatisticsSampleStride = 4;
        bool m_RequireUpdation       = true;
    };

} // namespace tf3d::generators
