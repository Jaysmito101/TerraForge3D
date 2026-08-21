#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Exporters/Serializer.h"
#include "Generators/Filters/BiomeFilterDefinition.h"
#include "Generators/GeneratorData.h"
#include "Generators/Masks/MaskLayer.h"
#include "Inspector/CustomInspectorSnapshot.h"

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class BiomeFilter
    {
    public:
        struct State {
            bool enabled                   = true;
            bool useMask                   = false;
            bool invertMask                = false;
            float strength                 = 1.0f;
            BiomeFilterMergeMode mergeMode = BiomeFilterMergeMode::Blend;
            inspector::CustomInspectorSnapshot values;
            MaskLayer::State mask;

            State(bool enabledState,
                  bool useMaskState,
                  bool invertMaskState,
                  float strengthState,
                  BiomeFilterMergeMode mergeModeState,
                  inspector::CustomInspectorSnapshot inspectorState,
                  MaskLayer::State maskState)
                : enabled(enabledState),
                  useMask(useMaskState),
                  invertMask(invertMaskState),
                  strength(strengthState),
                  mergeMode(mergeModeState),
                  values(std::move(inspectorState)),
                  mask(std::move(maskState))
            {
            }
        };
        using Snapshot = base::GeneratorState<State>::Snapshot;

        BiomeFilter(tf3d::data::ApplicationState *appState,
                    std::shared_ptr<BiomeFilterDefinition> definition);
        ~BiomeFilter() = default;

        bool ShowSettings();
        void Resize(int size);
        bool UpdateGeneratedMask(const State &state, GeneratorData *sourceData);
        void Load(SerializerNode data);
        SerializerNode Save() const;

        Snapshot GetState() const;
        inline Snapshot::Revision GetStateRevision() const
        {
            return m_State.PublishedRevision();
        }
        inline bool RequireUpdation() const
        {
            return m_State.RequiresUpdate();
        }

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
        inline BiomeFilterImplementation GetImplementation() const
        {
            return m_Definition->GetImplementation();
        }
        inline std::shared_ptr<BiomeFilterDefinition> GetDefinition() const
        {
            return m_Definition;
        }
        inline const inspector::CustomInspectorValue *FindParameterMetadata(const std::string &name) const
        {
            return m_Inspector == nullptr ? nullptr : m_Inspector->Root().Find(name);
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
        State CaptureState() const;
        void PublishState();

        data::ApplicationState *m_AppState = nullptr;
        std::shared_ptr<BiomeFilterDefinition> m_Definition;
        std::shared_ptr<inspector::CustomInspector> m_Inspector;
        std::string m_ID;
        bool m_InspectorReady = false;

        std::shared_ptr<MaskLayer> m_MaskLayer;
        State m_UIState;
        base::GeneratorState<State> m_State;
    };

} // namespace tf3d::generators
