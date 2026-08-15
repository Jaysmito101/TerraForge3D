#pragma once

#include "Base/Base.h"
#include "Base/RevisionTracker.h"
#include "Exporters/Serializer.h"
#include "Generators/GeneratorData.h"
#include "Generators/GeneratorTexture.h"
#include "Generators/GenerationContext.h"
#include "Generators/Masks/MaskLayer.h"
#include "Inspector/CustomInspector.h"
#include "Inspector/CustomInspectorSnapshot.h"

#include <memory>
#include <optional>
#include <utility>

TF3D_FWD_DEC_CLASS(ApplicationState, tf3d::data)

namespace tf3d::generators
{

    class BaseNoiseGenerator
    {
    public:
        struct State {
            bool enabled    = true;
            bool useMask    = true;
            bool invertMask = false;
            inspector::CustomInspectorSnapshot values;
            MaskLayer::State mask;

            State(bool enabledState,
                  bool useMaskState,
                  bool invertMaskState,
                  inspector::CustomInspectorSnapshot inspectorState,
                  MaskLayer::State maskState)
                : enabled(enabledState),
                  useMask(useMaskState),
                  invertMask(invertMaskState),
                  values(std::move(inspectorState)),
                  mask(std::move(maskState))
            {
            }
        };
        using Snapshot = base::GeneratorState<State>::Snapshot;

        explicit BaseNoiseGenerator(tf3d::data::ApplicationState *appState);

        bool Initialize();
        bool ShowEnabledControl(bool showRecommendation = false);
        bool ShowSettings();
        void Resize(int size);
        void Update(const Snapshot *state,
                    const GenerationContext *context,
                    GeneratorData *sourceBuffer,
                    GeneratorData *targetBuffer);

        inline Snapshot GetState() const
        {
            return m_State.Capture();
        }
        inline Snapshot::Revision GetStateRevision() const
        {
            return m_State.PublishedRevision();
        }

        void Load(SerializerNode data);
        SerializerNode Save();

        inline bool RequireUpdation() const
        {
            return m_State.RequiresUpdate();
        }

    private:
        State CaptureState() const;
        void PublishState();

    private:
        data::ApplicationState *m_AppState = nullptr;
        std::optional<base::ComputeShader> m_Shader;
        std::shared_ptr<inspector::CustomInspector> m_Inspector;
        std::shared_ptr<MaskLayer> m_MaskLayer;
        State m_UIState;
        base::GeneratorState<State> m_State;
    };

} // namespace tf3d::generators
