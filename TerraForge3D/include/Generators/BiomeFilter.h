#pragma once

#include "Base/Base.h"
#include "Exporters/Serializer.h"
#include "Generators/BiomeFilterDefinition.h"
#include "Generators/GeneratorData.h"
#include "Generators/Masks/MaskLayer.h"

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    enum class BiomeFilterMergeMode {
        Override,
        Add,
        Subtract,
        Multiply,
        Blend,
    };

    class BiomeFilter
    {
    public:
        BiomeFilter(tf3d::data::ApplicationState *appState,
                    std::shared_ptr<BiomeFilterDefinition> definition);
        ~BiomeFilter() = default;

        bool ShowSettings();
        void Resize(int size);
        void UpdateGeneratedMask(GeneratorData *sourceData);
        void Load(SerializerNode data);
        SerializerNode Save() const;

        inline const std::string &GetID() const
        {
            return m_ID;
        }
        inline const std::string &GetName() const
        {
            return m_Definition->GetName();
        }
        inline const std::string &GetCategory() const
        {
            return m_Definition->GetCategory();
        }
        inline const std::string &GetDescription() const
        {
            return m_Definition->GetDescription();
        }
        inline const std::string &GetDefinitionID() const
        {
            return m_Definition->GetID();
        }
        inline const std::string &GetImplementation() const
        {
            return m_Definition->GetImplementation();
        }
        inline std::shared_ptr<BiomeFilterDefinition> GetDefinition() const
        {
            return m_Definition;
        }
        inline const inspector::CustomInspectorValue *FindParameter(const std::string &name) const
        {
            return m_Inspector == nullptr ? nullptr : m_Inspector->Root().Find(name);
        }
        inline bool IsEnabled() const
        {
            return m_Enabled;
        }
        inline bool UsesMask() const
        {
            return m_UseMask;
        }
        inline bool InvertsMask() const
        {
            return m_InvertMask;
        }
        inline bool NeedsFieldStatistics() const
        {
            return m_Definition != nullptr && m_Definition->NeedsFieldStatistics();
        }
        inline bool NeedsHistogram() const
        {
            return m_Definition != nullptr && m_Definition->NeedsHistogram();
        }
        inline std::string GetRequestedPercentileParameter() const
        {
            return m_Definition != nullptr ? m_Definition->GetRequestedPercentileParameter() : std::string();
        }
        int GetIntegerParameter(const std::string &name, int defaultValue = 0) const;
        float GetFloatParameter(const std::string &name, float defaultValue = 0.0f) const;
        inline float GetStrength() const
        {
            return m_Strength;
        }
        inline BiomeFilterMergeMode GetMergeMode() const
        {
            return m_MergeMode;
        }
        inline GeneratorTexture *GetMaskTexture() const
        {
            return m_MaskLayer != nullptr ? m_MaskLayer->GetTexture() : nullptr;
        }
        inline base::ComputeShader *GetPhaseShader(data::ApplicationState *appState,
                                                   const std::string &phase) const
        {
            return m_Definition->GetPhaseShader(appState, phase);
        }

    private:
        data::ApplicationState *m_AppState = nullptr;
        std::shared_ptr<BiomeFilterDefinition> m_Definition;
        std::shared_ptr<inspector::CustomInspector> m_Inspector;
        std::string m_ID;

        bool m_Enabled                   = true;
        bool m_UseMask                   = false;
        bool m_InvertMask                = false;
        float m_Strength                 = 1.0f;
        BiomeFilterMergeMode m_MergeMode = BiomeFilterMergeMode::Blend;

        std::shared_ptr<MaskLayer> m_MaskLayer;
    };

} // namespace tf3d::generators
