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
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class BiomeID
    {
    public:
        BiomeID() = default;
        explicit BiomeID(std::string value)
            : m_Value(std::move(value))
        {
        }

        inline const std::string &GetValue() const noexcept
        {
            return m_Value;
        }

        inline const char *c_str() const noexcept
        {
            return m_Value.c_str();
        }

        friend bool operator==(const BiomeID &, const BiomeID &) = default;

    private:
        std::string m_Value;
    };

    struct BiomeIDHash {
        std::size_t operator()(const BiomeID &id) const noexcept
        {
            return std::hash<std::string>{}(id.GetValue());
        }
    };

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
            bool updateRequired                       = true;
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

        struct Runtime {
            BiomeID id;
            std::string name;
            std::shared_ptr<GeneratorData> data;
            std::shared_ptr<GeneratorTexture> maskTexture;
            std::vector<std::shared_ptr<BiomeBaseShapeGenerator>> baseShapeGenerators;
            std::shared_ptr<DEMBaseShapeGenerator> demBaseShapeGenerator;
            std::shared_ptr<BaseNoiseGenerator> baseNoiseGenerator;
            std::shared_ptr<BiomeCustomizeBaseShape> customBaseShape;
            std::shared_ptr<BiomeFilterStack> filterStack;
            std::shared_ptr<MaskLayer> maskLayer;
        };

        struct Snapshot {
            State state;
            Runtime runtime;
        };

        BiomeManager(data::ApplicationState *appState);
        ~BiomeManager();

        void Resize();
        Snapshot CaptureSnapshot() const;
        static bool Execute(const Snapshot *snapshot,
                            const GenerationContext *context,
                            GeneratorData *swapBuffer);
        void MarkProcessed(Revision revision);
        // bool ShowSettings();
        void ShowBaseShapeSettings();
        void ShowCustomizeBaseShapeSettings();
        inline void ShowCustomBaseShapeSettings()
        {
            ShowCustomizeBaseShapeSettings();
        }
        void ShowGeneralSettings();
        void ShowBaseNoiseSettings();
        void ShowMaskToolSettings();
        void ShowFilterSettings(int filterIndex);

        inline const bool IsEnabled() const
        {
            return m_IsEnabled;
        }
        inline const char *GetBiomeName() const
        {
            return m_BiomeName;
        }
        inline const bool IsUsingCustomBaseShape() const
        {
            return m_CustomizeBaseShape != nullptr && m_CustomizeBaseShape->IsEnabled();
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
        inline const BiomeID &GetBiomeID() const
        {
            return m_BiomeID;
        }
        inline void SetName(const std::string &name)
        {
            strcpy(m_BiomeName, name.c_str());
        }
        int AddFilter(const std::shared_ptr<BiomeFilterDefinition> &definition);
        bool RemoveFilter(int filterIndex);
        bool LoadUpResources();

    private:
        void MarkUpdateRequired();

        char m_BiomeName[64];
        bool m_IsEnabled = true;
        ImVec4 m_Color;
        BiomeID m_BiomeID;
        data::ApplicationState *m_AppState = nullptr;
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
        std::atomic_bool m_StatisticsDirty = true;
        int m_StatisticsSampleStride       = 4;
    };

} // namespace tf3d::generators
