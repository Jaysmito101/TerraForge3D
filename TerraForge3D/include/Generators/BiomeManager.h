#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Generators/BaseNoiseGenerator.h"
#include "Generators/BiomeBaseShapeGenerator.h"
#include "Generators/BiomeCustomBaseShape.h"
#include "Generators/Filters/BiomeFilterStack.h"
#include "Generators/DEMBaseShapeGenerator.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorDataStatistics.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/GenerationContext.h"
#include "Generators/Masks/MaskLayer.h"

#include <atomic>
#include <optional>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    enum BiomeBaseShapeGeneratorMode {
        BiomeBaseShapeGeneratorMode_Algorithm = 0,
        BiomeBaseShapeGeneratorMode_GlobalElevation,
        BiomeBaseShapeGeneratorMode_Count
    };

    static std::vector<std::string> s_BaseShapeGeneratorModeNames = {"Algorithm", "Global Elevation"};

    class BiomeManager
    {
    public:
        using Revision = base::RevisionTracker::Revision;

        struct State {
            Revision revision                         = 0;
            bool enabled                              = true;
            BiomeBaseShapeGeneratorMode baseShapeMode = BiomeBaseShapeGeneratorMode_Algorithm;
            int32_t baseShapeGenerator                = 0;
            std::optional<BiomeBaseShapeGenerator::Snapshot> baseShape;
            std::optional<DEMBaseShapeGenerator::Snapshot> demBaseShape;
            std::optional<BaseNoiseGenerator::Snapshot> baseNoise;
            std::optional<BiomeCustomizeBaseShape::Snapshot> customBaseShape;
            std::optional<BiomeFilterStack::Snapshot> filters;
            std::optional<MaskLayer::Snapshot> mask;
        };

        BiomeManager(tf3d::data::ApplicationState *appState);
        ~BiomeManager();

        void Resize();
        State GetState() const;
        void Update(const State *state, const GenerationContext *context, GeneratorData *swapBuffer);
        // bool ShowSettings();
        bool ShowBaseShapeSettings();
        bool ShowCustomizeBaseShapeSettings();
        inline bool ShowCustomBaseShapeSettings()
        {
            return ShowCustomizeBaseShapeSettings();
        }
        bool ShowGeneralSettings();
        bool ShowBaseNoiseSettings();
        bool ShowMaskToolSettings();
        bool ShowFilterSettings(int filterIndex);

        inline const bool IsEnabled() const
        {
            return m_IsEnabled;
        }
        inline const char *GetBiomeName() const
        {
            return m_BiomeName;
        }
        inline const bool IsUpdationRequired() const
        {
            return m_UpdateTracker.RequiresUpdate();
        }
        inline bool IsUpdationRequired(const State &state) const
        {
            return state.revision != m_UpdateTracker.ProcessedRevision();
        }
        inline const bool IsUsingCustomBaseShape() const
        {
            return m_CustomizeBaseShape != nullptr && m_CustomizeBaseShape->IsEnabled();
        }
        inline GeneratorData *GetBiomeData() const
        {
            return m_Data.get();
        }
        inline const ImVec4 &GetColor() const
        {
            return m_Color;
        }
        inline const int GetFiltersCount() const
        {
            return static_cast<int>(m_FilterStack->GetFilters().size());
        }
        inline const std::vector<std::shared_ptr<BiomeFilter>> &GetFilters() const
        {
            return m_FilterStack->GetFilters();
        }
        inline const std::vector<std::shared_ptr<BiomeFilterDefinition>> &GetFilterDefinitions() const
        {
            return m_FilterStack->GetDefinitions();
        }
        inline const std::string &GetBiomeID() const
        {
            return m_BiomeID;
        }
        inline void SetName(const std::string &name)
        {
            strcpy(m_BiomeName, name.c_str());
        }
        inline GeneratorTexture *GetMaskTexture() const
        {
            return m_MaskLayer != nullptr ? m_MaskLayer->GetTexture() : nullptr;
        }
        inline GeneratorTexture *GetMaskPreviewTexture() const
        {
            return m_MaskLayer != nullptr ? m_MaskLayer->GetTexture() : nullptr;
        }

        int AddFilter(const std::shared_ptr<BiomeFilterDefinition> &definition);
        bool RemoveFilter(int filterIndex);
        bool LoadUpResources();

    private:
        void MarkUpdateRequired();

        char m_BiomeName[64];
        bool m_IsEnabled = true;
        ImVec4 m_Color;
        std::string m_BiomeID                    = "";
        tf3d::data::ApplicationState *m_AppState = nullptr;
        std::shared_ptr<GeneratorData> m_Data;
        base::RevisionTracker m_UpdateTracker{1};
        std::atomic<int32_t> m_SelectedBaseShapeGenerator                         = 0;
        std::atomic<BiomeBaseShapeGeneratorMode> m_SelectedBaseShapeGeneratorMode = BiomeBaseShapeGeneratorMode_Algorithm;
        std::shared_ptr<BiomeFilterStack> m_FilterStack;
        std::shared_ptr<DEMBaseShapeGenerator> m_DEMBaseShapeGenerator;
        std::shared_ptr<MaskLayer> m_MaskLayer;

        std::vector<std::shared_ptr<BiomeBaseShapeGenerator>> m_BaseShapeGenerators;
        std::shared_ptr<BaseNoiseGenerator> m_BaseNoiseGenerator;
        std::shared_ptr<BiomeCustomizeBaseShape> m_CustomizeBaseShape;
        std::shared_ptr<GeneratorDataStatistics> m_Statistics;
        GeneratorDataStatisticsResult m_StatisticsResult;
        bool m_StatisticsDirty       = true;
        int m_StatisticsSampleStride = 4;
    };

} // namespace tf3d::generators
